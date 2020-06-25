#include "Header.hlsli"
#include "Light.hlsli"

float4 main(VertexOut pin) : SV_TARGET
{
    int2 posS;
    posS = pin.posH - 0.5f;
    InterlockedAdd(uabSum[posS], 1);
    
    float4 normalW = normalize(pin.normalW);

    float4 lightFactor = float4(calLights(pin.posW, normalW), 1.0f);

    float3 unlightColor = { 0.2f, 0.2f, 0.2f };
    float4 color = float4(unlightColor, 0.0f) + lightFactor*gDiffuse;
    return color;
}