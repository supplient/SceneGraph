#include "Header.hlsli"

float4 main(VertexOut pin) : SV_TARGET
{
	return float4(pin.normalW.xyz, 1.0f);
}