#ifndef HEADER_H
#define HEADER_H
#include "Predefine.hlsli"

struct DirLight
{
    float4 color;
    float4 direction;
    float4x4 viewMat;
    float4x4 projMat;
    uint id;
    float3 padding1;
};

struct PointLight
{
    float4 color;
    float4 pos;
    uint id;
    float rmin;
    float r0;
    float padding1;
};

struct SpotLight
{
    float4 color;
    float4 pos;
    float4 direction;
    float4x4 viewMat;
    float4x4 projMat;
    uint id;
    float rmin;
    float r0;
    float penumbra;
    float umbra;
    float3 padding1;
};

struct RectLight
{
    float4 color;
    float4 verts[4];
    float rmin;
    float r0;
    float2 padding1;
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
};

struct RenderParams
{
    float3 normalW;
    float3 viewW;
    float3 ambient;
    float3 baseColor;
    float metalness;
    float ior;
    float roughness;
};

#define NUM_CONTROL_POINTS 3

cbuffer cbPerObject: register(b0)
{
    float4x4 gModelMat;
    float4x4 gNormalModelMat;
};

cbuffer cbPerMaterial : register(b1)
{
    float4 gBaseColor;

    float gMetalness;
    float gIOR;
    float gRoughness;

    uint gLTCMatTexID;
    uint gLTCAmpTexID;

    uint gBaseColorTexID;
    uint gDispTexID;
    float gDispHeightScale;
    uint gNormalTexID;
    float gAlphaTestTheta;
};

cbuffer cbPerPass : register(b2)
{
    float4 gEyePosW;
    float4x4 gViewMat;
    float4x4 gProjMat;
    uint4 gLightPerTypeNum; // direction, point, spot, rect
    DirLight gDirLights[MAX_DIR_LIGHT_NUM];
    PointLight gPointLights[MAX_POINT_LIGHT_NUM];
    SpotLight gSpotLights[MAX_SPOT_LIGHT_NUM];
    RectLight gRectLights[MAX_RECT_LIGHT_NUM];
};

// UAVs
// NCount for weighted-average
RWTexture2D<uint> uabNCount : register(u0);

// ZBuffer for referce
#ifdef MULTIPLE_SAMPLE
    Texture2DMS<float> srbZBuffer : register(t0);
#else
    Texture2D srbZBuffer : register(t0);
#endif

// Textures // This does not follow the name convention, for convenience
#if TEXTURE_NUM > 0
    Texture2D gTexs[TEXTURE_NUM] : register(t1, space0);
#endif
#if SPOT_SHADOW_TEX_NUM > 0
    Texture2D gSpotShadowTexs[SPOT_SHADOW_TEX_NUM] : register(t0, space1);
#endif
#if DIR_SHADOW_TEX_NUM > 0
    Texture2D gDirShadowTexs[DIR_SHADOW_TEX_NUM] : register(t0, space2);
#endif
#if POINT_SHADOW_TEX_NUM > 0
    TextureCube gPointShadowTexs[POINT_SHADOW_TEX_NUM] : register(t0, space3);
#endif

// Samplers
SamplerState bilinearWrap : register(s0);
SamplerState nearestBorder : register(s1);

#endif//HEADER_H