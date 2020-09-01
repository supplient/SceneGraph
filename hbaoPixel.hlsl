#include "Predefine.hlsli"
#include "MultiSampleLoader.hlsli"

cbuffer cbHBAO : register(b0)
{
    float gScaleX;
    float gScaleY;
    float gNearZ;
    float gFarZ;
    uint gClientWidth;
    uint gClientHeight;
    float2 padding1;
};

#ifdef MULTIPLE_SAMPLE
    // TODO
    Texture2DMS<float> depthTex : register(t0);
    Texture2DMS<float> colorTex : register(t1);
#else
    Texture2D depthTex : register(t0);
    Texture2D colorTex : register(t1);
#endif

float3 posS2posE(int2 posS, uint sampleIndex)
{
    posS.x = clamp(posS.x, 0, gClientWidth);
    posS.y = clamp(posS.y, 0, gClientHeight);

    float depth = LoadFloat4(depthTex, posS, sampleIndex).r;
    float2 uv = (float2) posS / float2(gClientWidth, gClientHeight);
    uv = uv * 2.0f - 1.0f;

    float x = uv.x * gScaleX;
    float y = -uv.y * gScaleY;
    float z = depth * (gFarZ - gNearZ) + gNearZ;
    return float3(x, y, z);
}

float3 minDiff(float3 pl, float3 p, float3 pr)
{
    float3 lv = pl - p;
    float3 rv = p - pr;
    float lvLen = dot(lv, lv);
    float rvLen = dot(rv, rv);
    if(lvLen > rvLen)
        return rv;
    else
        return lv;
}

float3 reconstructNormal(int2 posS, uint sampleIndex)
{
    float3 p = posS2posE(posS, sampleIndex);
    float3 pl = posS2posE(posS - int2(1, 0), sampleIndex);
    float3 pr = posS2posE(posS + int2(1, 0), sampleIndex);
    float3 pu = posS2posE(posS - int2(0, 1), sampleIndex);
    float3 pd = posS2posE(posS + int2(0, 1), sampleIndex);
    float3 normal = cross(minDiff(pl, p, pr), minDiff(pu, p, pd));
    return normalize(normal);
}

float4 main(float4 posH : SV_POSITION, uint sampleIndex : SV_SampleIndex) : SV_TARGET
{
    int2 posS;
    posS = floor(posH.xy);

    float4 lightColor = LoadFloat4(colorTex, posS, sampleIndex);
    float3 normal = reconstructNormal(posS, sampleIndex);
    
    return float4(normal, 1.0f);
}