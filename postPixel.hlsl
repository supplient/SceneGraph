Texture2D opaque : register(t0);
RWTexture2D<uint> sum : register(u0);

float4 main(float4 posH : SV_POSITION) : SV_TARGET
{
    int2 posS;
    posS = posH.xy - 0.5f;
    float4 opaqueColor = opaque.Load(int3(posS, 0));
    uint s = sum.Load(posS);
    if (s == 0)
        return float4(0.7, 0.3, 0.5, 1.0);
    else if (s == 1)
        return opaqueColor;
    else
        return float4(1.0, 0.7, 0.2, 1.0);
}