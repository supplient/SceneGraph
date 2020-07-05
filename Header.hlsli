#ifndef HEADER_H
#define HEADER_H
#include "Predefine.hlsli"

struct Light
{
    float4 color;
    float4 pos;
    float4 direction;
};

struct VertexIn
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 tex : TEXTURE;
};

struct VertexOut
{
    float4 posW : POSITION;
    float4 normalW : NORMAL;
    float2 tex : TEXTURE;
    float4 posH : SV_POSITION;
};

cbuffer cbPerObject: register(b0)
{
    float4x4 gModelMat;
    float4x4 gNormalModelMat;
};

cbuffer cbPerMaterial : register(b1)
{
    float4 gDiffuse;
    int gDiffuseTexID;
};

cbuffer cbPerPass : register(b2)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    uint4 gLightPerTypeNum; // direction, point, spot, padding
    Light gLights[MAX_LIGHT_NUM];
};

// NCount for weighted-average
RWTexture2D<uint> uabNCount : register(u0);

// ZBuffer for referce
#ifdef MULTIPLE_SAMPLE
    Texture2DMS<float> srbZBuffer : register(t0);
#else
    Texture2D srbZBuffer : register(t0);
#endif

// Textures
Texture2D srbTexs[TEXTURE_NUM] : register(t1);

// Samplers
SamplerState bilinearWrap : register(s0);

#endif//HEADER_H