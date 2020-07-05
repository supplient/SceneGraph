#include "Header.hlsli"
#include "Light.hlsli"

float4 main(VertexOut pin) : SV_TARGET
{
    float4 normalW = normalize(pin.normalW);

    float4 lightFactor = float4(calLights(pin.posW, normalW), 1.0f);

    float4 diffuseColor;
    if (gDiffuseTexID > 0)
    {
        diffuseColor = gTexs[gDiffuseTexID - 1].Sample(bilinearWrap, pin.tex);
    }
    else
    {
        diffuseColor = gDiffuse;
    }

    float3 unlightColor = { 0.2f, 0.2f, 0.2f };
    float4 color = float4(unlightColor, 0.0f) + lightFactor*diffuseColor;
    return color;
}