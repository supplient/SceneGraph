cbuffer cbPerObject : register(b0)
{
    float4x4 gModelMat;
}

cbuffer cbPerPass : register(b1)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
};

float4 main( float4 posL : POSITION ) : SV_POSITION
{
    return mul(mul(mul(posL, gModelMat), gViewMat), gProjMat);
}