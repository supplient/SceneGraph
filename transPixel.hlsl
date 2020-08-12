#include "MultiSampleLoader.hlsli"
#include "Header.hlsli"
#include "Light.hlsli"

float4 main(VertexOut pin, uint sampleIndex: SV_SampleIndex) : SV_TARGET
{
    int2 posS;
    posS = floor(pin.posH.xy);
    float baseZ = LoadFloat(srbZBuffer, posS, sampleIndex).x;
    clip(baseZ - pin.posH.z);
    float4 viewW = normalize(gEyePosW - pin.posW);

    int2 nCountPos = posS;
#ifdef MULTIPLE_SAMPLE
    nCountPos.x = 4*nCountPos.x + sampleIndex;
#endif
    InterlockedAdd(uabNCount[nCountPos], 1);
    
    float4 normalW = normalize(pin.normalW);

    float3 lightColor = calLights(pin.posW, normalW, viewW, gDiffuse);

    float3 unlightColor = { 0.2f, 0.2f, 0.2f };
    float3 color = unlightColor + lightColor;

    return float4(color, gDiffuse.a);
}
