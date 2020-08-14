#ifndef LTC_HEADER
#define LTC_HEADER

float2 calLTCUV(float3 viewDir, float3 normal, float roughness)
{
    float2 ltcUV;

    float viewTheta = acos(dot(viewDir, normal));
    ltcUV = float2(roughness, viewTheta / (0.5 * 3.14159));

    const float LTC_LUT_SIZE = 32.0;
    // scale and bias coordinates, for correct filtered lookup
    ltcUV = ltcUV * (LTC_LUT_SIZE - 1.0) / LTC_LUT_SIZE + 0.5 / LTC_LUT_SIZE;

    return ltcUV;
}

void loadLTCMatAndAmp(
    out float3x3 ltcMatInv, out float ltcAmp,
    float3 viewDir, float3 normal,
    Texture2D ltcMatTex, Texture2D ltcAmpTex, SamplerState ltcTexSampler,
    float roughness
    )
{
    float2 ltcUV = calLTCUV(viewDir, normal, roughness);
    
    float4 ltcMatInvParams = ltcMatTex.Sample(ltcTexSampler, ltcUV);
    ltcMatInv = float3x3(
        1, 0, ltcMatInvParams.y,
        0, ltcMatInvParams.z, 0,
        ltcMatInvParams.w, 0, ltcMatInvParams.x
    );
    ltcAmp = ltcAmpTex.Sample(ltcTexSampler, ltcUV).x;
}

#endif//LTC_HEADER