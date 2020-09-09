#include "Header.hlsli"
#include "Light.hlsli"
#include "HelpFunctions.hlsli"

float4 main(VertexOut pin) : SV_TARGET
{
    float4 bitanW = float4(cross(pin.normalW.xyz, pin.tangentW.xyz), 0.0f);
    
    float4 normalW = normalize(pin.normalW);
    float4 tanW = normalize(pin.tangentW);
    bitanW = normalize(bitanW);
    float4x4 TBNTransMat;
    TBNTransMat[0] = tanW;
    TBNTransMat[1] = bitanW;
    TBNTransMat[2] = normalW;
    TBNTransMat[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4x4 TBNMat = transpose(TBNTransMat);

    float2 uv = pin.tex;
    float4 viewW = normalize(gEyePosW - pin.posW);

#if TEXTURE_NUM > 0
    if (IsValidTexID(gNormalTexID))
    {
        float3 texNormal = gTexs[gNormalTexID].Sample(bilinearWrap, uv).xyz;
        texNormal.xyz = texNormal.xyz * 2 - 1.0f;
        normalW = normalize(mul(float4(texNormal, 0.0f), TBNTransMat));
    }
#endif

    float4 baseColor = gBaseColor;
#if TEXTURE_NUM > 0
    if (IsValidTexID(gBaseColorTexID))
    {
        baseColor = gTexs[gBaseColorTexID].Sample(bilinearWrap, uv);
    }
#endif

    if (gAlphaTestTheta < 0.999)
    {
        clip(baseColor.a - gAlphaTestTheta);
    }

    RenderParams rps;
    rps.normalW = normalW.xyz;
    rps.viewW = viewW.xyz;
    rps.ambient = float3(0.2f, 0.2f, 0.2f);
    rps.baseColor = baseColor.xyz;
    rps.metalness = gMetalness;
    rps.ior = gIOR;
    rps.roughness = gRoughness;
    float4 lightColor = float4(calLights(
        pin.posW, rps
    ), 1.0f);

    float4 color = lightColor;
    return color;
}