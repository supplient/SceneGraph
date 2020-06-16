#include "Header.hlsli"

VertexOut main( VertexIn vin )
{
    VertexOut vout;

    float4x4 mvpMat = mul(mul(gModelMat, gViewMat), gProjMat);

    float4 posL = float4(vin.posL, 1.0);
    vout.posW = mul(posL, gModelMat);
    vout.posH = mul(mul(vout.posW, gViewMat), gProjMat);
    vout.normalW = mul(float4(vin.normalL, 0.0), gNormalModelMat);
    
	return vout;
}