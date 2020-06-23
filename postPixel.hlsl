#include "Header.hlsli"

float4 main(float4 posH : SV_POSITION) : SV_TARGET
{
    int2 posS;
    posS = posH.xy - 0.5f;
    uint s = sum.Load(posS);
    if(s == 0)
	    return float4(0.0f, 0.0f, 0.0f, 0.0f);
    else if(s == 1)
	    return float4(1.0f, 0.0f, 0.0f, 1.0f);
    else if(s == 2)
	    return float4(0.0f, 1.0f, 0.0f, 1.0f);
    else if(s == 3)
	    return float4(0.0f, 0.0f, 1.0f, 1.0f);
    else
	    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}