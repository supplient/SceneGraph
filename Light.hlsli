#ifndef LIGHT_H
#define LIGHT_H

#include "Predefine.hlsli"
#include "Header.hlsli"
#include "BRDF.hlsli"

float3 calPhongModel(float3 lightDir, float3 lightColor, RenderParams rps, float3 fresnel0, float3 ssAlbedo)
{
    float lambCos = calLambCos(lightDir, rps.normalW);
    float3 halfVec = normalize(lightDir + rps.viewW);
    float m = (1 - rps.roughness) * 1024.0;
    float roughnessFactor = (m + 8.0f) / 8.0f * pow(max(dot(rps.normalW, halfVec), 0.0f), m);

    // diffuse
    float3 diffuseBRDF = ssAlbedo;

    // specular
    float3 specularBRDF = calFresnelReflectance(rps.normalW, lightDir, fresnel0) * roughnessFactor;
    
    return rps.ambient * rps.baseColor + lightColor * lambCos * (diffuseBRDF + specularBRDF);
}

float3 calShadingEquation(
    float3 lightDir, float3 lightColor, RenderParams rps)
{
    float3 nonmetalF0 = calFresnel0(rps.ior);
    float3 fresnel0 = rps.metalness * rps.baseColor + (1.0f - rps.metalness) * nonmetalF0;
    float3 ssAlbedo = rps.metalness * float3(0.0f, 0.0f, 0.0f) + (1 - rps.metalness) * rps.baseColor;
#if SHADING_MODEL == 2
    if (IsValidTexID(gLTCAmpTexID) && IsValidTexID(gLTCMatTexID))
    {
        // This model needs a light scale-up
        lightColor *= 8.0f;
    
        // BRDF calculation
        float3 specularBRDF, diffuseBRDF;
        calBRDF_punctual(specularBRDF, diffuseBRDF,
            lightDir, rps,
            ssAlbedo, fresnel0
        );
        return rps.ambient * rps.baseColor + lightColor * (specularBRDF + diffuseBRDF);
    }
    else
        return calPhongModel(lightDir, lightColor, rps, fresnel0, ssAlbedo);

#elif SHADING_MODEL == 1
    return calPhongModel(lightDir, lightColor, rps, fresnel0, ssAlbedo);

#else
    return rps.ambient*rps.baseColor + lightColor * rps.baseColor * calLambCos(lightDir, rps.normalW);

#endif
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

float3 calLights(
    float4 posW, RenderParams rps
)
{
    float3 sum = { 0.0f, 0.0f, 0.0f };
    uint i = 0;

    // direction lights
    for (i = 0; i < gLightPerTypeNum.x; i++)
    {
        // Cal Light Color
        // Shadow Test
        float shadowFactor = 1.0f;
#if DIR_SHADOW_TEX_NUM > 0
        if (IsValidLightID(gDirLights[i].id))
        {
            float4 posLi = mul(mul(posW, gDirLights[i].viewMat), gDirLights[i].projMat);
            posLi.xyz = posLi.xyz / posLi.w;
            posLi.w = 1.0f;
            float2 shadowUV = (posLi.xy + 1.0f) / 2.0f;
            shadowUV.y = 1.0f - shadowUV.y;
            float occluderDepth = gDirShadowTexs[gDirLights[i].id].Sample(nearestBorder, shadowUV).r;
            float receiverDepth = posLi.z;
            if(receiverDepth > occluderDepth)
                shadowFactor = 0.0f;
        }
#endif
        float3 lightColor = shadowFactor * gDirLights[i].color.xyz;
        
        sum += calShadingEquation(
            gDirLights[i].direction.xyz,
            lightColor,
            rps
        );
    }

    // point lights
    for (i = 0; i < gLightPerTypeNum.y; i++)
    {
        float4 dir = gPointLights[i].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        // Cal Light Color
        // Shadow Test
        float shadowFactor = 1.0f;
#if POINT_SHADOW_TEX_NUM > 0
        if (IsValidLightID(gPointLights[i].id))
        {
            float3 shadowUVW = (posW - gPointLights[i].pos).xyz;
            float occluderDepth = gPointShadowTexs[gPointLights[i].id].Sample(nearestBorder, shadowUVW).r;
            float receiverDepth = length(shadowUVW);
            if(receiverDepth > occluderDepth)
                shadowFactor = 0.0f;
        }
#endif
        // Light Attenuation
        float distAtte = calDistAttenuation(dist, gPointLights[i].r0, gPointLights[i].rmin);
        float3 lightColor = shadowFactor * distAtte * gPointLights[i].color.xyz;

        sum += calShadingEquation(
            dir.xyz,
            lightColor,
            rps
        );
    }

    // spot lights
    for (i = 0; i < gLightPerTypeNum.z; i++)
    {
        float4 dir = gSpotLights[i].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        // Cal Light Color
        // Shadow Test
        float shadowFactor = 1.0f;
#if SPOT_SHADOW_TEX_NUM > 0
        if (IsValidLightID(gSpotLights[i].id))
        {
            float4 posLi = mul(mul(posW, gSpotLights[i].viewMat), gSpotLights[i].projMat);
            posLi.xyz = posLi.xyz / posLi.w;
            posLi.w = 1.0f;
            float2 shadowUV = (posLi.xy + 1.0f) / 2.0f;
            shadowUV.y = 1.0f - shadowUV.y;
            float occluderDepth = gSpotShadowTexs[gSpotLights[i].id].Sample(nearestBorder, shadowUV).r;
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

        sum += calShadingEquation(
            dir.xyz,
            lightColor,
            rps
        );
    }
    
    // rect lights
#   if RECT_LIGHT_ON
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
        float3 centerPos = gRectLights[i].verts[0].xyz;
        centerPos += gRectLights[i].verts[1].xyz;
        centerPos += gRectLights[i].verts[2].xyz;
        centerPos += gRectLights[i].verts[3].xyz;
        centerPos /= 4.0f;
        float dist = length(centerPos - posW.xyz);
        float distAtte = calDistAttenuation(dist, gRectLights[i].r0, gRectLights[i].rmin);
        float3 lightColor = distAtte * gRectLights[i].color.xyz;

        sum += lightColor * specularBRDF + lightColor * diffuseBRDF;
    }
#   endif
   
    return sum;
}

#endif// LIGHT_H