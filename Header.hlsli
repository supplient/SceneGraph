#ifndef BUFFER_HEADER_H
#define BUFFER_HEADER_H

#define MAX_LIGHT_NUM 16

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
};

struct VertexOut
{
    float4 posW : POSITION;
    float4 normalW : NORMAL;
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
};

cbuffer cbPerPass : register(b2)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    uint4 gLightPerTypeNum; // direction, point, spot, padding
    Light gLights[MAX_LIGHT_NUM];
};

RWTexture2D<uint> uabNCount : register(u0);
Texture2D srbZBuffer : register(t0);

#endif//BUFFER_HEADER_H