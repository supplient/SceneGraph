#include "pointShadowHeader.hlsli"

cbuffer cbPerObject : register(b0)
{
    float4x4 gModelMat;
}

cbuffer cbPerPass : register(b1)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
};

VertexOut main( float4 posL : POSITION )
{
    VertexOut res;
    res.posLi = mul(mul(posL, gModelMat), gViewMat);
    res.posH = mul(res.posLi, gProjMat);
    return res;
}