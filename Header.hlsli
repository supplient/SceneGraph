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
    float3 tangentL : TANGENT;
    float2 tex : TEXTURE;
};

struct VertexOut
{
    float4 posW : POSITION;
    float4 normalW : NORMAL;
    float4 tangentW : TANGENT;
    float2 tex : TEXTURE;
    float4 posH : SV_POSITION;
};

struct HullConstant
{
	float EdgeTessFactor[3]			: SV_TessFactor; // 例如，对于四象限域，将为 [4]
	float InsideTessFactor			: SV_InsideTessFactor; // 例如，对于四象限域，将为 Inside[2]
	// TODO:  更改/添加其他资料
};

#define NUM_CONTROL_POINTS 3

cbuffer cbPerObject: register(b0)
{
    float4x4 gModelMat;
    float4x4 gNormalModelMat;
};

cbuffer cbPerMaterial : register(b1)
{
    float4 gDiffuse;
    uint gDiffuseTexID;
    uint gDispTexID;
    float gDispHeightScale;
    uint gNormalTexID;
};

cbuffer cbPerPass : register(b2)
{
    float4 gEyePosW;
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

// Textures // This does not follow the name convention, for convenience
Texture2D gTexs[TEXTURE_NUM] : register(t1);

// Samplers
SamplerState bilinearWrap : register(s0);

#endif//HEADER_H