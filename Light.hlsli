#ifndef LIGHT_H
#define LIGHT_H

#include "Header.hlsli"

float calLambCos(float4 dir, float4 normal)
{
    float lambCos = dot(dir, normal);
    return clamp(lambCos, 0.0f, 1.0f);
}

float3 calBRDFwithLambCosPunctual(
    float3 viewDir, float3 normal,
    Texture2D ltcMatTex, Texture2D ltcAmpTex,
    float3 lightDir
    )
{
    // cal TBNMat
    // Note: TBNMat here is different from TBNMat outside
    float3 T1, T2;
    T1 = normalize(viewDir - normal * dot(viewDir, normal));
    T2 = cross(normal, T1);
    float3x3 TBNMat = float3x3(T1, T2, normal);
    TBNMat = transpose(TBNMat);
    
    // cal LTC tex's coordinate
    float2 ltcUV;
    {
        float viewTheta = acos(dot(viewDir, normal));
        ltcUV = float2(gRoughness, viewTheta / (0.5 * 3.14159));

        const float LTC_LUT_SIZE = 32.0;
        // scale and bias coordinates, for correct filtered lookup
        ltcUV = ltcUV * (LTC_LUT_SIZE - 1.0) / LTC_LUT_SIZE + 0.5 / LTC_LUT_SIZE;
    }

    // load LTC mat && amp
    float3x3 ltcMatInv;
    float ltcAmp;
    {
        float4 ltcMatInvParams = ltcMatTex.Sample(bilinearWrap, ltcUV);
        ltcMatInv = float3x3(
            1, 0, ltcMatInvParams.y,
            0, ltcMatInvParams.z, 0,
            ltcMatInvParams.w, 0, ltcMatInvParams.x
        );
        ltcAmp = ltcAmpTex.Sample(bilinearWrap, ltcUV).x;
    }

    // Transform lightDir
    float3 lightDirL = mul(lightDir, TBNMat);
    lightDirL = mul(lightDirL, ltcMatInv);
    lightDirL = normalize(lightDirL);
    
    // Cal standard cosine lobe
    float cosLobe = max(0, lightDirL.z) / 3.14159;

    // Multiple LTC lobe's magnitude
    cosLobe *= ltcAmp;

    // Multiple Fresnel reflectance, F0
    return cosLobe * gSpecular.xyz;
}

float calDistAttenuation(float dist, float r0, float rmin)
{
    return pow(r0/max(dist, rmin), 2);
}

float calDirAttenuation(float4 defDir, float4 dir, float penumbra, float umbra)
{
    float dirCos = dot(defDir, dir);
    float t = (dirCos - umbra) / (penumbra - umbra);
    return smoothstep(0.0f, 1.0f, t);
}

float3 calLights(float4 posW, float4 normalW, float4 viewW, float4 diffuseColor)
{
    float3 sum = { 0.0f, 0.0f, 0.0f };
    uint i = 0;

    // direction lights
    for (i = 0; i < gLightPerTypeNum.x; i++)
    {
        // BRDF calculation
        // specular
        float3 specularBRDF;
        if (gLTCAmpTexID > 0 && gLTCMatTexID > 0)
        {
            specularBRDF = calBRDFwithLambCosPunctual(
                viewW.xyz, normalW.xyz,
                gTexs[gLTCMatTexID - 1], gTexs[gLTCAmpTexID - 1],
                gDirLights[i].direction.xyz
            );
        }
        else
            specularBRDF = gSpecular.xyz * calLambCos(gDirLights[i].direction, normalW);
        // diffuse
        // TODO diffuse should be calculated by concerning energy conservation
        float3 diffuseBRDF = diffuseColor.xyz * calLambCos(gDirLights[i].direction, normalW);

        // Cal Light Color
        // Shadow Test
        float shadowFactor = 1.0f;
#if DIR_SHADOW_TEX_NUM > 0
        if (gDirLights[i].id > 0)
        {
            float4 posLi = mul(mul(posW, gDirLights[i].viewMat), gDirLights[i].projMat);
            posLi.xyz = posLi.xyz / posLi.w;
            posLi.w = 1.0f;
            float2 shadowUV = (posLi.xy + 1.0f) / 2.0f;
            shadowUV.y = 1.0f - shadowUV.y;
            float occluderDepth = gDirShadowTexs[gDirLights[i].id - 1].Sample(nearestBorder, shadowUV).r;
            float receiverDepth = posLi.z;
            if(receiverDepth > occluderDepth)
                shadowFactor = 0.0f;
        }
#endif
        float3 lightColor = shadowFactor * gDirLights[i].color.xyz;
        
        sum += lightColor * specularBRDF + lightColor * diffuseBRDF;
    }

    // point lights
    for (i = 0; i < gLightPerTypeNum.y; i++)
    {
        float4 dir = gPointLights[i].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        // BRDF calculation
        // specular
        float3 specularBRDF;
        if (gLTCAmpTexID > 0 && gLTCMatTexID > 0)
        {
            specularBRDF = calBRDFwithLambCosPunctual(
                viewW.xyz, normalW.xyz,
                gTexs[gLTCMatTexID - 1], gTexs[gLTCAmpTexID - 1],
                dir.xyz
            );
        }
        else
            specularBRDF = gSpecular.xyz * calLambCos(dir, normalW);
        // diffuse
        float3 diffuseBRDF = diffuseColor.xyz * calLambCos(dir, normalW);

        // Cal Light Color
        // Shadow Test
        float shadowFactor = 1.0f;
#if POINT_SHADOW_TEX_NUM > 0
        if (gPointLights[i].id > 0)
        {
            float3 shadowUVW = (posW - gPointLights[i].pos).xyz;
            float occluderDepth = gPointShadowTexs[gPointLights[i].id - 1].Sample(nearestBorder, shadowUVW).r;
            float receiverDepth = length(shadowUVW);
            if(receiverDepth > occluderDepth)
                shadowFactor = 0.0f;
        }
#endif
        float distAtte = calDistAttenuation(dist, gPointLights[i].r0, gPointLights[i].rmin);
        float3 lightColor = shadowFactor * distAtte * gPointLights[i].color.xyz;

        sum += lightColor * specularBRDF + lightColor * diffuseBRDF;
    }

    // spot lights
    for (i = 0; i < gLightPerTypeNum.z; i++)
    {
        float4 dir = gSpotLights[i].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        // BRDF calculation
        // specular
        float3 specularBRDF;
        if (gLTCAmpTexID > 0 && gLTCMatTexID > 0)
        {
            specularBRDF = calBRDFwithLambCosPunctual(
                viewW.xyz, normalW.xyz,
                gTexs[gLTCMatTexID - 1], gTexs[gLTCAmpTexID - 1],
                dir.xyz
            );
        }
        else
            specularBRDF = gSpecular.xyz * calLambCos(dir, normalW);
        // diffuse
        float3 diffuseBRDF = diffuseColor.xyz * calLambCos(dir, normalW);

        // Cal Light Color
        // Shadow Test
        float shadowFactor = 1.0f;
#if SPOT_SHADOW_TEX_NUM > 0
        if (gSpotLights[i].id > 0)
        {
            float4 posLi = mul(mul(posW, gSpotLights[i].viewMat), gSpotLights[i].projMat);
            posLi.xyz = posLi.xyz / posLi.w;
            posLi.w = 1.0f;
            float2 shadowUV = (posLi.xy + 1.0f) / 2.0f;
            shadowUV.y = 1.0f - shadowUV.y;
            float occluderDepth = gSpotShadowTexs[gSpotLights[i].id - 1].Sample(nearestBorder, shadowUV).r;
            float receiverDepth = posLi.z;
            if(receiverDepth > occluderDepth)
                shadowFactor = 0.0f;
        }
#endif
        float lambCos = calLambCos(dir, normalW);
        float distAtte = calDistAttenuation(dist, gSpotLights[i].r0, gSpotLights[i].rmin);
        float dirAtte = calDirAttenuation(
            gSpotLights[i].direction, 
            dir,
            gSpotLights[i].penumbra,
            gSpotLights[i].umbra
        );
        float3 lightColor = shadowFactor * dirAtte * distAtte * gSpotLights[i].color.xyz;

        sum += lightColor * specularBRDF + lightColor * diffuseBRDF;
    }
    
    return sum;
}

#endif// LIGHT_H