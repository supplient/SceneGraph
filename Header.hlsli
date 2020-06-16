#ifndef HEADER_H
#define HEADER_H

cbuffer cbPerObject: register(b0)
{
    float4x4 gModelMat;
};

cbuffer cbPerPass : register(b1)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
}

struct VertexIn
{
    float3 posL : POS;
};

struct VertexOut
{
    float4 posW : POS;
    float4 posH : SV_POSITION;
};

#endif // HEADER_H
