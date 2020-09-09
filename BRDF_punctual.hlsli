#ifndef BRDF_PUNCTUAL_HEADER
#define BRDF_PUNCTUAL_HEADER

#include "BRDF_common.hlsli"
#include "HelpFunctions.hlsli"


float3 calSpecularBRDFWithCos_punctual(
    float3 viewDir, float3 normal,
    Texture2D ltcMatTex, Texture2D ltcAmpTex,
    float3 lightDir,
    float3 fresnel0
    )
{
    // cal TBNMat
    // Note: TBNMat here is different from TBNMat outside
    float3x3 TBNMat = calTBNofVirtualNormal(viewDir, normal);

    // load LTC mat && amp
    float3x3 ltcMatInv;
    float ltcAmp;
    loadLTCMatAndAmp(ltcMatInv, ltcAmp,
        viewDir, normal,
        ltcMatTex, ltcAmpTex, bilinearWrap,
        gRoughness
    );

    // Transform lightDir
    float3 lightDirL = mul(lightDir, TBNMat);
    lightDirL = mul(lightDirL, ltcMatInv);
    lightDirL = normalize(lightDirL);
    
    // Cal standard cosine lobe
    float cosLobe = max(0, lightDirL.z) / 3.14159;

    // Multiple LTC lobe's magnitude
    cosLobe *= ltcAmp;

    // Calculate Fresnel reflectance, using Schlick approximation
    float3 fresnel = calFresnelReflectance(normalize(lightDir+normal), lightDir, fresnel0);

    // Multiple Fresnel reflectance
    return cosLobe * fresnel;
}

float3 calDiffuseBRDFWithCos_punctual(
    float3 viewDir, float3 normal, float3 lightDir, 
    float3 ssAlbedo, float3 fresnel0
    )
{
    // Using Shirley approximation
    float3 res = ssAlbedo * (1 - fresnel0);
    res *= 1 - pow(1 - max(0, dot(normal, lightDir)), 5);
    res *= 1 - pow(1 - max(0, dot(normal, viewDir)), 5);
    res *= 21.0f / 20.0f / 3.14159;
    res *= calLambCos(lightDir, normal);
    return res;
}

void calBRDF_punctual(out float3 out_specular, out float3 out_diffuse, 
    float3 viewDir, float3 normal, float3 lightDir, 
    float3 ssAlbedo, float3 fresnel0
    )
{
    // specular
    if (IsValidTexID(gLTCAmpTexID) && IsValidTexID(gLTCMatTexID))
    {
        out_specular = calSpecularBRDFWithCos_punctual(
            viewDir, normal,
            gTexs[gLTCMatTexID], gTexs[gLTCAmpTexID],
            lightDir,
            fresnel0
        );
    }
    else
        out_specular = calFresnelReflectance(normal, lightDir, fresnel0) * calLambCos(lightDir, normal);

    // diffuse
    out_diffuse = calDiffuseBRDFWithCos_punctual(viewDir, normal, lightDir, ssAlbedo, fresnel0);
}

#endif//BRDF_PUNCTUAL_HEADER