#include "Header.hlsli"
#include "Light.hlsli"

float4 main(VertexOut pin, uint sampleIndex: SV_SampleIndex) : SV_TARGET
{
    int2 posS;
    posS = floor(pin.posH.xy);
    float baseZ = srbZBuffer.Load(int2(posS), sampleIndex).x;
    clip(baseZ - pin.posH.z);
    InterlockedAdd(uabNCount[posS], 1);
    
    float4 normalW = normalize(pin.normalW);

    float3 lightFactor = calLights(pin.posW, normalW);

    float3 unlightColor = { 0.2f, 0.2f, 0.2f };
    float3 color = unlightColor + lightFactor*gDiffuse.xyz;

    return float4(color, gDiffuse.a);
}
