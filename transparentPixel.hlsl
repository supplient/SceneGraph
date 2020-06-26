#include "Header.hlsli"
#include "Light.hlsli"

float4 main(VertexOut pin) : SV_TARGET
{
    int2 posS;
    posS = floor(pin.posH.xy);
    float baseZ = srbZBuffer.Load(int3(posS, 0)).x;
    clip(baseZ - pin.posH.z);
    InterlockedAdd(uabNCount[posS], 1);
    
    float4 normalW = normalize(pin.normalW);

    float3 lightFactor = calLights(pin.posW, normalW);

    float3 unlightColor = { 0.2f, 0.2f, 0.2f };
    float3 color = unlightColor + lightFactor*gDiffuse.xyz;

    return float4(color, gDiffuse.a);
}
