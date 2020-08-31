#include "Predefine.hlsli"
#include "MultiSampleLoader.hlsli"

cbuffer cbHBAO : register(b0)
{
	// TODO
};

SamplerState bilinearWrap : register(s0);

#ifdef MULTIPLE_SAMPLE
    // TODO
    Texture2DMS<float> depthTex : register(t0);
    Texture2DMS<float> colorTex : register(t1);
#else
    Texture2D depthTex : register(t0);
    Texture2D colorTex : register(t1);
#endif

float4 main(float4 posH : SV_POSITION, uint sampleIndex : SV_SampleIndex) : SV_TARGET
{
    int2 posS;
    posS = floor(posH.xy);
    float4 lightColor = LoadFloat4(colorTex, posS, sampleIndex);
    return lightColor;
}