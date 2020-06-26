Texture2DMS<float4> opaque : register(t0);
Texture2DMS<float4> trans : register(t1);
RWTexture2D<uint> nCount : register(u0);

float4 main(float4 posH : SV_POSITION, uint sampleIndex: SV_SampleIndex) : SV_TARGET
{
    int2 posS;
    posS = floor(posH.xy);
    float4 opaqueColor = opaque.Load(posS, sampleIndex);
    uint n = nCount.Load(posS);
    if(n == 0)
        return float4(opaqueColor.xyz, 1.0);

    float4 transValue = trans.Load(posS, sampleIndex);
    float3 transColor = transValue.xyz;
    float transAlpha = transValue.a;

    float3 avgColor = transColor / transAlpha;
    float avgAlpha = transAlpha / n;
    float u = pow(1 - avgAlpha, n);
    float3 color = (1 - u) * avgColor + u * opaqueColor.xyz;

    return float4(color, 1.0);
}