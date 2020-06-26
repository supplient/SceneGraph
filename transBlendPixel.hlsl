#include "MultiSampleLoader.hlsli"

#ifdef MULTIPLE_SAMPLE
    Texture2DMS<float4> opaque : register(t0);
    Texture2DMS<float4> trans : register(t1);
#else
    Texture2D opaque : register(t0);
    Texture2D trans : register(t1);
#endif//MULTIPLE_SAMPLE

RWTexture2D<uint> nCount : register(u0);

float4 main(float4 posH : SV_POSITION, uint sampleIndex : SV_SampleIndex) : SV_TARGET
{
    int2 posS;
    posS = floor(posH.xy);
    float4 opaqueColor = LoadFloat4(opaque, posS, sampleIndex);
    uint n = nCount.Load(posS);
#ifdef MULTIPLE_SAMPLE
    float nFloat = n / 4.0f;
    n = ceil(nFloat);
#endif//MULTIPLE_SAMPLE

    float4 transValue = LoadFloat4(trans, posS, sampleIndex);
    float transAlpha = transValue.a;
    if(transAlpha == 0.0f)
        return float4(opaqueColor.xyz, 1.0);
    float3 transColor = transValue.xyz;

    float3 avgColor = transColor / transAlpha;
    float avgAlpha = transAlpha / n;
    float u = pow(1 - avgAlpha, n);
    float3 color = (1 - u) * avgColor + u * opaqueColor.xyz;

    return float4(color, 1.0);
}