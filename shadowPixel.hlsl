float4 main(float4 posH:SV_Position) : SV_TARGET
{
    return float4(posH.z, posH.x, posH.y, 1.0f);
}