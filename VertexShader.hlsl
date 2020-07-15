#include "Header.hlsli"

VertexOut main( VertexIn vin )
{
    VertexOut vout;

    float4x4 mvpMat = mul(mul(gModelMat, gViewMat), gProjMat);

    float4 posL = float4(vin.posL, 1.0);
    vout.posW = mul(posL, gModelMat);
    vout.posH = mul(mul(vout.posW, gViewMat), gProjMat);
    vout.tangentW = mul(float4(vin.tangentL, 0.0f), gModelMat);
    vout.normalW = normalize(mul(float4(vin.normalL, 0.0), gNormalModelMat));
    // TODO some bug with normalW
    vout.tex = vin.tex;
    
	return vout;
}