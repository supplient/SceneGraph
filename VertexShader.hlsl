#include "Header.hlsli"

VertexOut main( VertexIn vin )
{
    VertexOut vout;

    float4 posL = float4(vin.posL, 1.0);
    vout.posW = mul(posL, gModelMat);
    vout.posH = mul(mul(vout.posW, gViewMat), gProjMat);
	return vout;
}