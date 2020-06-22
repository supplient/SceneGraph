#ifndef HEADER_H
#define HEADER_H

#define MAX_LIGHT_NUM 16

struct Light
{
    float4 color;
    float4 pos;
    float4 direction;
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
}

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

#endif // HEADER_H
