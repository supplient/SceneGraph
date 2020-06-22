#include "Header.hlsli"

float3 calLights(float4 posW, float4 normalW)
{
    float3 sum = { 0.0f, 0.0f, 0.0f };
    uint li = 0;
    uint i = 0;

    // direction lights
    for (i = 0; i < gLightPerTypeNum.x; i++)
    {
        float lambCos = dot(gLights[li].direction, normalW);
        lambCos = clamp(lambCos, 0.0f, 1.0f);
        sum +=  lambCos * gLights[li].color.xyz;
        li++;
    }

    // point lights
    for (i = 0; i < gLightPerTypeNum.y; i++)
    {
        float4 dir = normalize(gLights[li].pos - posW);
        float lambCos = dot(dir, normalW);
        lambCos = clamp(lambCos, 0.0f, 1.0f);
        sum += lambCos * gLights[li].color.xyz;
        li++;
    }
    
    return sum;
}

float4 main(VertexOut pin) : SV_TARGET
{
    float4 normalW = normalize(pin.normalW);

    float4 lightFactor = float4(calLights(pin.posW, normalW), 1.0f);

    float3 unlightColor = { 0.2f, 0.2f, 0.2f };
    float4 color = float4(unlightColor, 0.0f) + lightFactor*gDiffuse;
    return color;
}