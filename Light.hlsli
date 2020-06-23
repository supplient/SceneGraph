#ifndef LIGHT_HEADER
#define LIGHT_HEADER

#include "Header.hlsli"

float calLambCos(float4 dir, float4 normal)
{
    float lambCos = dot(dir, normal);
    return clamp(lambCos, 0.0f, 1.0f);
}

float calDistAttenuation(float dist)
{
    // r0 = 0.5f
    // rmin = 0.01f
    // TODO these two params may should assign by light itself
    return pow(0.5f/max(dist, 0.01f), 2);
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
    uint li = 0;
    uint i = 0;

    // direction lights
    for (i = 0; i < gLightPerTypeNum.x; i++)
    {
        float lambCos = calLambCos(gLights[li].direction, normalW);
        sum += lambCos * gLights[li].color.xyz;
        li++;
    }

    // point lights
    for (i = 0; i < gLightPerTypeNum.y; i++)
    {
        float4 dir = gLights[li].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        float lambCos = calLambCos(dir, normalW);
        float distAtte = calDistAttenuation(dist);
        sum += distAtte * lambCos * gLights[li].color.xyz;
        li++;
    }

    // spot lights
    for (i = 0; i < gLightPerTypeNum.z; i++)
    {
        float4 dir = gLights[li].pos - posW;
        float dist = length(dir);
        dir = normalize(dir);

        float lambCos = calLambCos(dir, normalW);
        float distAtte = calDistAttenuation(dist);
        float dirAtte = calDirAttenuation(gLights[li].direction, dir);
        sum += dirAtte * distAtte * lambCos * gLights[li].color.xyz;
        li++;
    }
    
    return sum;
}

#endif