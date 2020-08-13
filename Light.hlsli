#ifndef LIGHT_H
#define LIGHT_H

#include "Header.hlsli"

float calLambCos(float3 dir, float3 normal)
{
    float lambCos = dot(dir, normal);
    return clamp(lambCos, 0.0f, 1.0f);
}

float3 calFresnelReflectance(float3 normal, float3 lightDir)
{
    float3 fresnel = gSpecular.xyz;
    fresnel = fresnel + (1 - fresnel) * pow(1 - max(0, dot(normal, lightDir)), 5);
    return fresnel;
}

float3x3 calTBNofVirtualNormal(float3 viewDir, float3 normal)
{
    float3 T1, T2;
    T1 = normalize(viewDir - normal * dot(viewDir, normal));
    T2 = cross(normal, T1);
    float3x3 TBNMat = float3x3(T1, T2, normal);
    TBNMat = transpose(TBNMat);
    return TBNMat;
}

float2 calLTCUV(float3 viewDir, float3 normal)
{
    float2 ltcUV;

    float viewTheta = acos(dot(viewDir, normal));
    ltcUV = float2(gRoughness, viewTheta / (0.5 * 3.14159));

    const float LTC_LUT_SIZE = 32.0;
    // scale and bias coordinates, for correct filtered lookup
    ltcUV = ltcUV * (LTC_LUT_SIZE - 1.0) / LTC_LUT_SIZE + 0.5 / LTC_LUT_SIZE;

    return ltcUV;
}

void loadLTCMatAndAmp(
    out float3x3 ltcMatInv, out float ltcAmp,
    float3 viewDir, float3 normal,
    Texture2D ltcMatTex, Texture2D ltcAmpTex
    )
{
    float2 ltcUV = calLTCUV(viewDir, normal);
    
    float4 ltcMatInvParams = ltcMatTex.Sample(bilinearWrap, ltcUV);
    ltcMatInv = float3x3(
        1, 0, ltcMatInvParams.y,
        0, ltcMatInvParams.z, 0,
        ltcMatInvParams.w, 0, ltcMatInvParams.x
    );
    ltcAmp = ltcAmpTex.Sample(bilinearWrap, ltcUV).x;
}

float3 calSpecularBRDFWithCos_punctual(
    float3 viewDir, float3 normal,
    Texture2D ltcMatTex, Texture2D ltcAmpTex,
    float3 lightDir
    )
{
    // cal TBNMat
    // Note: TBNMat here is different from TBNMat outside
    float3x3 TBNMat = calTBNofVirtualNormal(viewDir, normal);

    // load LTC mat && amp
    float3x3 ltcMatInv;
    float ltcAmp;
    loadLTCMatAndAmp(ltcMatInv, ltcAmp,
        viewDir, normal,
        ltcMatTex, ltcAmpTex
    );

    // Transform lightDir
    float3 lightDirL = mul(lightDir, TBNMat);
    lightDirL = mul(lightDirL, ltcMatInv);
    lightDirL = normalize(lightDirL);
    
    // Cal standard cosine lobe
    float cosLobe = max(0, lightDirL.z) / 3.14159;

    // Multiple LTC lobe's magnitude
    cosLobe *= ltcAmp;

    // Calculate Fresnel reflectance, using Schlick approximation
    float3 fresnel = calFresnelReflectance(normal, lightDir);

    // Multiple Fresnel reflectance
    return cosLobe * fresnel;
}

float3 clipLineAtHorizon(float3 up, float3 down)
{
    float x = (up.x * down.z - down.x * up.z) / (down.z - up.z);
    float y = (up.y * down.z - down.y * up.z) / (down.z - up.z);
    return normalize(float3(x, y, 0.0f));
}

/*
int clipUnderHorizon(inout float3 ps0, inout float3 ps1, inout float3 ps2, inout float3 ps3, inout float3 ps4)
{
    return 4;
}
*/

int clipUnderHorizon(inout float3 ps0, inout float3 ps1, inout float3 ps2, inout float3 ps3, inout float3 ps4)
{
    bool ah0, ah1, ah2, ah3; // above horizon
    ah0 = ps0.z >= 0;
    ah1 = ps1.z >= 0;
    ah2 = ps2.z >= 0;
    ah3 = ps3.z >= 0;

    float3 cs0, cs1, cs2, cs3; // a copy, for convenience
    cs0 = ps0;
    cs1 = ps1;
    cs2 = ps2;
    cs3 = ps3;

    // Oh shit, there are 16 conditions
    // All points down
    if(!ah0 && !ah1 && !ah2 && !ah3)
    {
        return 0;
    }
    // One point up
    else if(ah0 && !ah1 && !ah2 && !ah3)
    {
        // v0 clip v1 v3
        ps1 = clipLineAtHorizon(cs0, cs1);
        ps2 = clipLineAtHorizon(cs0, cs3);
        return 3;
    }
    else if(!ah0 && ah1 && !ah2 && !ah3)
    {
        // v1 clip v0 v2
        ps0 = clipLineAtHorizon(cs1, cs0);
        ps2 = clipLineAtHorizon(cs1, cs2);
        return 3;
    }
    else if(!ah0 && !ah1 && ah2 && !ah3)
    {
        // v2 clip v1 v3
        ps0 = clipLineAtHorizon(cs2, cs3);
        ps1 = clipLineAtHorizon(cs2, cs1);
        return 3;
    }
    else if(!ah0 && !ah1 && !ah2 && ah3)
    {
        // v3 clip v0 v2
        ps0 = cs3;
        ps1 = clipLineAtHorizon(cs3, cs0);
        ps2 = clipLineAtHorizon(cs3, cs2);
        return 3;
    }
    // Two points up
    else if(ah0 && ah1 && !ah2 && !ah3)
    {
        // v0 v1 clip v2 v3
        ps2 = clipLineAtHorizon(cs1, cs2);
        ps3 = clipLineAtHorizon(cs0, cs3);
        return 4;
    }
    else if(!ah0 && ah1 && ah2 && !ah3)
    {
        // v1 v2 clip v3 v0
        ps3 = clipLineAtHorizon(cs2, cs3);
        ps0 = clipLineAtHorizon(cs1, cs0);
        return 4;
    }
    else if(!ah0 && !ah1 && ah2 && ah3)
    {
        // v2 v3 clip v0 v1
        ps0 = clipLineAtHorizon(cs3, cs0);
        ps1 = clipLineAtHorizon(cs2, cs1);
        return 4;
    }
    else if(ah0 && !ah1 && !ah2 && ah3)
    {
        // v3 v0 clip v1 v2
        ps1 = clipLineAtHorizon(cs0, cs1);
        ps2 = clipLineAtHorizon(cs3, cs2);
        return 4;
    }
    // Three points up
    else if(ah0 && ah1 && ah2 && !ah3)
    {
        // v0 v1 v2 clip v3
        ps0 = cs1;
        ps1 = cs2;
        ps2 = clipLineAtHorizon(cs2, cs3);
        ps3 = clipLineAtHorizon(cs0, cs3);
        ps4 = cs0;
        return 5;
    }
    else if(!ah0 && ah1 && ah2 && ah3)
    {
        // v1 v2 v3 clip v0
        ps0 = cs2;
        ps1 = cs3;
        ps2 = clipLineAtHorizon(cs3, cs0);
        ps3 = clipLineAtHorizon(cs1, cs0);
        ps4 = cs1;
        return 5;
    }
    else if(ah0 && !ah1 && ah2 && ah3)
    {
        // v2 v3 v0 clip v1
        ps0 = cs3;
        ps1 = cs0;
        ps2 = clipLineAtHorizon(cs0, cs1);
        ps3 = clipLineAtHorizon(cs2, cs1);
        ps4 = cs2;
        return 5;
    }
    else if(ah0 && ah1 && !ah2 && ah3)
    {
        // v3 v0 v1 clip v2
        ps0 = cs0;
        ps1 = cs1;
        ps2 = clipLineAtHorizon(cs1, cs2);
        ps3 = clipLineAtHorizon(cs3, cs2);
        ps4 = cs3;
        return 5;
    }
    // All points up
    else
    {
        return 4;
    }
}

float calStandardCosLobe(float3 ps0, float3 ps1, float3 ps2, float3 ps3, float3 ps4, int n)
{
    float cosLobe = 0.0f;

    float tmp = 0.0f;

    if(n==0)
    {
        return 0.0f;
    }

    tmp = acos(dot(ps0, ps1));
    tmp *= dot(normalize(cross(ps0, ps1)), float3(0, 0, 1));
    cosLobe += tmp;

    tmp = acos(dot(ps1, ps2));
    tmp *= dot(normalize(cross(ps1, ps2)), float3(0, 0, 1));
    cosLobe += tmp;

    if(n==3)
    {
        tmp = acos(dot(ps2, ps0));
        tmp *= dot(normalize(cross(ps2, ps0)), float3(0, 0, 1));
        cosLobe += tmp;

        cosLobe = cosLobe / 2.0f / 3.14159f;
        return cosLobe;
    }

    tmp = acos(dot(ps2, ps3));
    tmp *= dot(normalize(cross(ps2, ps3)), float3(0, 0, 1));
    cosLobe += tmp;

    if(n==4)
    {
        tmp = acos(dot(ps3, ps0));
        tmp *= dot(normalize(cross(ps3, ps0)), float3(0, 0, 1));
        cosLobe += tmp;

        cosLobe = cosLobe / 2.0f / 3.14159f;
        return cosLobe;
    }

    tmp = acos(dot(ps3, ps4));
    tmp *= dot(normalize(cross(ps3, ps4)), float3(0, 0, 1));
    cosLobe += tmp;

    // n==5
    tmp = acos(dot(ps4, ps0));
    tmp *= dot(normalize(cross(ps4, ps0)), float3(0, 0, 1));
    cosLobe += tmp;

    cosLobe = cosLobe / 2.0f / 3.14159f;
    return cosLobe;
}

float3 calSpecularBRDFWithCos_rect(
    float3 viewDir, float3 normal,
    Texture2D ltcMatTex, Texture2D ltcAmpTex,
    float4 vertDirs[4]
    )
{
    // cal TBNMat
    // Note: TBNMat here is different from TBNMat outside
    float3x3 TBNMat = calTBNofVirtualNormal(viewDir, normal);

    // load LTC mat && amp
    float3x3 ltcMatInv;
    float ltcAmp;
    loadLTCMatAndAmp(ltcMatInv, ltcAmp,
        viewDir, normal,
        ltcMatTex, ltcAmpTex
    );

    // transform vertDirs
    ltcMatInv = mul(TBNMat, ltcMatInv);
    float3 vd0, vd1, vd2, vd3, vd4;
    vd0 = normalize(mul(vertDirs[0].xyz, ltcMatInv));
    vd1 = normalize(mul(vertDirs[1].xyz, ltcMatInv));
    vd2 = normalize(mul(vertDirs[2].xyz, ltcMatInv));
    vd3 = normalize(mul(vertDirs[3].xyz, ltcMatInv));
    vd4 = float3(0.0f, 0.0f, 0.0f);

    // clip, remove parts under horizon
    int n = 0;
    n = clipUnderHorizon(vd0, vd1, vd2, vd3, vd4);

    // Cal standard cosine lobe integration
    float cosLobe = calStandardCosLobe(vd0, vd1, vd2, vd3, vd4, n);
    cosLobe = max(-cosLobe, 0.0f);

    // Multiple LTC lobe's magnitude
    cosLobe *= ltcAmp;

    // Calculate certerPos as an approximation for Fresnel
    float3 centerPos = vertDirs[0].xyz;
    centerPos += vertDirs[1].xyz;
    centerPos += vertDirs[2].xyz;
    centerPos += vertDirs[3].xyz;
    centerPos = normalize(centerPos);

    // Calculate Fresnel reflectance, using Schlick approximation
    float3 fresnel = calFresnelReflectance(normal, centerPos);

    // Multiple Fresnel reflectance
    return cosLobe * fresnel;
}

float3 calDiffuseBRDFWithCos_punctual(
    float3 viewDir, float3 normal, float3 lightDir, 
    float3 ssAlbedo
    )
{
    // Using Shirley approximation
    float3 res = ssAlbedo * (1 - gSpecular.xyz);
    res *= 1 - pow(1 - max(0, dot(normal, lightDir)), 5);
    res *= 1 - pow(1 - max(0, dot(normal, viewDir)), 5);
    res *= 21.0f / 20.0f / 3.14159;
    res *= calLambCos(lightDir, normal);
    return res;
}

void calBRDF_punctual(out float3 out_specular, out float3 out_diffuse, 
    float3 viewDir, float3 normal, float3 lightDir, 
    float3 ssAlbedo
    )
{
    // specular
    if (gLTCAmpTexID > 0 && gLTCMatTexID > 0)
    {
        out_specular = calSpecularBRDFWithCos_punctual(
            viewDir, normal,
            gTexs[gLTCMatTexID - 1], gTexs[gLTCAmpTexID - 1],
            lightDir
        );
    }
    else
        out_specular = calFresnelReflectance(normal, lightDir) * calLambCos(lightDir, normal);

    // diffuse
    out_diffuse = calDiffuseBRDFWithCos_punctual(viewDir, normal, lightDir, ssAlbedo);
}

void calBRDF_rect(out float3 out_specular, out float3 out_diffuse, 
    float3 viewDir, float3 normal, float4 vertDirs[4], 
    float3 ssAlbedo
    )
{
    // specular
    if (gLTCAmpTexID > 0 && gLTCMatTexID > 0)
    {
        out_specular = calSpecularBRDFWithCos_rect(
            viewDir, normal,
            gTexs[gLTCMatTexID - 1], gTexs[gLTCAmpTexID - 1],
            vertDirs
        );
    }
    else // TODO
        out_specular = gSpecular.xyz;

    // diffuse
    // TODO
    out_diffuse = ssAlbedo;
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
        float3 specularBRDF, diffuseBRDF;
        calBRDF_punctual(specularBRDF, diffuseBRDF, 
            viewW.xyz, normalW.xyz, gDirLights[i].direction.xyz, 
            diffuseColor.xyz
        );

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
        float3 specularBRDF, diffuseBRDF;
        calBRDF_punctual(specularBRDF, diffuseBRDF, 
            viewW.xyz, normalW.xyz, dir.xyz, 
            diffuseColor.xyz
        );

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
        // Light Attenuation
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
        float3 specularBRDF, diffuseBRDF;
        calBRDF_punctual(specularBRDF, diffuseBRDF, 
            viewW.xyz, normalW.xyz, dir.xyz, 
            diffuseColor.xyz
        );

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
        // Light Attenuation
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
    
    // rect lights
    for (i = 0; i < gLightPerTypeNum.w; i++)
    {
        // BRDF calculation
        float3 specularBRDF, diffuseBRDF;
        float4 vertDirs[4];
        int j = 0;
        for (j = 0; j < 4; j++)
        {
            vertDirs[j] = gRectLights[i].verts[j] - posW;
        }
        calBRDF_rect(specularBRDF, diffuseBRDF,
            viewW.xyz, normalW.xyz, vertDirs,
            diffuseColor.xyz
        );

        // Cal light color
        // Attenuation TODO
        float3 lightColor = gRectLights[i].color.xyz;

        sum += lightColor * specularBRDF + lightColor * diffuseBRDF;
    }
   
    return sum;
}

#endif// LIGHT_H