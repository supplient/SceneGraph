#include "pointShadowHeader.hlsli"

float4 main(VertexOut pin) : SV_TARGET
{
    float dist = length(pin.posLi.xyz);
    return float4(dist, 0.0f, 0.0f, 1.0f);

}