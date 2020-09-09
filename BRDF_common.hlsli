#ifndef BRDF_COMMON_HEADER
#define BRDF_COMMON_HEADER

#include "Header.hlsli"
#include "LTC.hlsli"


float3 calFresnel0(float ior)
{
    float k = pow((ior - 1) / (ior + 1), 2);
    return float3(k, k, k);
}

float calLambCos(float3 dir, float3 normal)
{
    float lambCos = dot(dir, normal);
    return clamp(lambCos, 0.0f, 1.0f);
}

float3 calFresnelReflectance(float3 normal, float3 lightDir, float3 fresnel0)
{
    float3 fresnel = fresnel0 + (1 - fresnel0) * pow(1 - max(0, dot(normal, lightDir)), 5);
    return fresnel;
}

float3x3 calTBNofVirtualNormal(float3 viewDir, float3 normal)
{
    float3 T1, T2;
    T1 = normalize(viewDir - normal * dot(viewDir, normal));
    T2 = cross(normal, T1);
    float3x3 TBNMat = float3x3(T1, T2, normal);
    TBNMat = transpose(TBNMat);
    return TBNMat;
}

#endif//BRDF_COMMON_HEADER