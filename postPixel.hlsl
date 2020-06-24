Texture2D opaque : register(t0);

float4 main(float4 posH : SV_POSITION) : SV_TARGET
{
    int2 posS;
    posS = posH.xy - 0.5f;
    float4 s = opaque.Load(int3(posS, 0));
    return s;
}