#ifndef BRDF_PUNCTUAL_HEADER
#define BRDF_PUNCTUAL_HEADER

#include "BRDF_common.hlsli"
#include "HelpFunctions.hlsli"


float3 calSpecularBRDFWithCos_punctual(
    RenderParams rps,
    Texture2D ltcMatTex, Texture2D ltcAmpTex,
    float3 lightDir,
    float3 fresnel0
    )
{
    // cal TBNMat
    // Note: TBNMat here is different from TBNMat outside
    float3x3 TBNMat = calTBNofVirtualNormal(rps.viewW, rps.normalW);

    // load LTC mat && amp
    float3x3 ltcMatInv;
    float ltcAmp;
    loadLTCMatAndAmp(ltcMatInv, ltcAmp,
        rps.viewW, rps.normalW,
        ltcMatTex, ltcAmpTex, bilinearWrap,
        rps.roughness
    );

    // Transform lightDir
    float3 lightDirL = mul(lightDir, TBNMat);
    lightDirL = mul(lightDirL, ltcMatInv);
    lightDirL = normalize(lightDirL);
    
    // Cal standard cosine lobe
    float cosLobe = max(0, lightDirL.z) / 3.14159;

    // Check origin lightDir
    if(dot(lightDir, rps.normalW) <= 0.0f)
        cosLobe = 0.0f;

    // Multiple LTC lobe's magnitude
    cosLobe *= ltcAmp;

    // Calculate Fresnel reflectance, using Schlick approximation
    float3 fresnel = calFresnelReflectance(normalize(lightDir+rps.normalW), lightDir, fresnel0);

    // Multiple Fresnel reflectance
    return cosLobe * fresnel;
}

float3 calDiffuseBRDFWithCos_punctual(
    RenderParams rps, float3 lightDir, 
    float3 ssAlbedo, float3 fresnel0
    )
{
    // Using Shirley approximation
    float3 res = ssAlbedo * (1 - fresnel0);
    res *= 1 - pow(1 - max(0, dot(rps.normalW, lightDir)), 5);
    res *= 1 - pow(1 - max(0, dot(rps.normalW, rps.viewW)), 5);
    res *= 21.0f / 20.0f / 3.14159;
    res *= calLambCos(lightDir, rps.normalW);
    return res;
}

void calBRDF_punctual(out float3 out_specular, out float3 out_diffuse, 
    float3 lightDir, RenderParams rps,
    float3 ssAlbedo, float3 fresnel0
    )
{
    // specular
    out_specular = calSpecularBRDFWithCos_punctual(
        rps,
        gTexs[gLTCMatTexID], gTexs[gLTCAmpTexID],
        lightDir,
        fresnel0
    );

    // diffuse
    out_diffuse = calDiffuseBRDFWithCos_punctual(rps, lightDir, ssAlbedo, fresnel0);
}

#endif//BRDF_PUNCTUAL_HEADER