#ifndef LIGHT_H
#define LIGHT_H

#include "Header.hlsli"

float calLambCos(float4 dir, float4 normal)
{
    float lambCos = dot(dir, normal);
    return clamp(lambCos, 0.0f, 1.0f);
}

float calDistAttenuation(float dist, float r0, float rmin)
{
    return pow(r0/max(dist, rmin), 2);
}

float calDirAttenuation(float4 defDir, float4 dir)
{
    // penumbra = 10 angle
    // umbra = 45 angle
    // TODO these two params may should assign by light itself
    float penumbra = 0.9848;
    float umbra = 0.707;
    float dirCos = dot(defDir, dir);
    float t = (dirCos - umbra) / (penumbra - umbra);
    return smoothstep(0.0f, 1.0f, t);
}

float3 calLights(float4 posW, float4 normalW)
{
    float3 sum = { 0.0f, 0.0f, 0.0f };
    uint i = 0;

    // direction lights
    for (i = 0; i < gLightPerTypeNum.x; i++)
    {
        float lambCos = calLambCos(gDirLights[i].direction, normalW);

        // Shadow Test
        float shadowFactor = 1.0f;
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

        sum += shadowFactor * lambCos * gDirLights[i].color.xyz;
    }

    // point lights
    for (i = 0; i < gLightPerTypeNum.y; i++)
    {
        float4 dir = gPointLights[i].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        // Shadow Test
        float shadowFactor = 1.0f;
        if (gPointLights[i].id > 0)
        {
            float3 shadowUVW = (posW - gPointLights[i].pos).xyz;
            float occluderDepth = gPointShadowTexs[gPointLights[i].id - 1].Sample(nearestBorder, shadowUVW).r;
            float receiverDepth = length(shadowUVW);
            if(receiverDepth > occluderDepth)
                shadowFactor = 0.0f;
        }

        float lambCos = calLambCos(dir, normalW);
        float distAtte = calDistAttenuation(dist, gPointLights[i].r0, gPointLights[i].rmin);
        sum += shadowFactor * distAtte * lambCos * gPointLights[i].color.xyz;
    }

    // spot lights
    for (i = 0; i < gLightPerTypeNum.z; i++)
    {
        float4 dir = gSpotLights[i].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        // Shadow Test
        float shadowFactor = 1.0f;
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

        float lambCos = calLambCos(dir, normalW);
        float distAtte = calDistAttenuation(dist, gSpotLights[i].r0, gSpotLights[i].rmin);
        float dirAtte = calDirAttenuation(gSpotLights[i].direction, dir);
        sum += shadowFactor * dirAtte * distAtte * lambCos * gSpotLights[i].color.xyz;
    }
    
    return sum;
}

#endif// LIGHT_H