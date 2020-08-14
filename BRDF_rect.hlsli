#ifndef BRDF_RECT_HEADER
#define BRDF_RECT_HEADER

#include "BRDF_common.hlsli"


float3 clipLineAtHorizon(float3 up, float3 down)
{
    float x = (up.x * down.z - down.x * up.z) / (down.z - up.z);
    float y = (up.y * down.z - down.y * up.z) / (down.z - up.z);
    return normalize(float3(x, y, 0.0f));
}

/*
int clipUnderHorizon(inout float3 ps0, inout float3 ps1, inout float3 ps2, inout float3 ps3, inout float3 ps4)
{
    return 4;
}
*/

int clipUnderHorizon(inout float3 ps0, inout float3 ps1, inout float3 ps2, inout float3 ps3, inout float3 ps4)
{
    bool ah0, ah1, ah2, ah3; // above horizon
    ah0 = ps0.z >= 0;
    ah1 = ps1.z >= 0;
    ah2 = ps2.z >= 0;
    ah3 = ps3.z >= 0;

    float3 cs0, cs1, cs2, cs3; // a copy, for convenience
    cs0 = ps0;
    cs1 = ps1;
    cs2 = ps2;
    cs3 = ps3;

    // Oh shit, there are 16 conditions
    // All points down
    if(!ah0 && !ah1 && !ah2 && !ah3)
    {
        return 0;
    }
    // One point up
    else if(ah0 && !ah1 && !ah2 && !ah3)
    {
        // v0 clip v1 v3
        ps1 = clipLineAtHorizon(cs0, cs1);
        ps2 = clipLineAtHorizon(cs0, cs3);
        return 3;
    }
    else if(!ah0 && ah1 && !ah2 && !ah3)
    {
        // v1 clip v0 v2
        ps0 = clipLineAtHorizon(cs1, cs0);
        ps2 = clipLineAtHorizon(cs1, cs2);
        return 3;
    }
    else if(!ah0 && !ah1 && ah2 && !ah3)
    {
        // v2 clip v1 v3
        ps0 = clipLineAtHorizon(cs2, cs3);
        ps1 = clipLineAtHorizon(cs2, cs1);
        return 3;
    }
    else if(!ah0 && !ah1 && !ah2 && ah3)
    {
        // v3 clip v0 v2
        ps0 = cs3;
        ps1 = clipLineAtHorizon(cs3, cs0);
        ps2 = clipLineAtHorizon(cs3, cs2);
        return 3;
    }
    // Two points up
    else if(ah0 && ah1 && !ah2 && !ah3)
    {
        // v0 v1 clip v2 v3
        ps2 = clipLineAtHorizon(cs1, cs2);
        ps3 = clipLineAtHorizon(cs0, cs3);
        return 4;
    }
    else if(!ah0 && ah1 && ah2 && !ah3)
    {
        // v1 v2 clip v3 v0
        ps3 = clipLineAtHorizon(cs2, cs3);
        ps0 = clipLineAtHorizon(cs1, cs0);
        return 4;
    }
    else if(!ah0 && !ah1 && ah2 && ah3)
    {
        // v2 v3 clip v0 v1
        ps0 = clipLineAtHorizon(cs3, cs0);
        ps1 = clipLineAtHorizon(cs2, cs1);
        return 4;
    }
    else if(ah0 && !ah1 && !ah2 && ah3)
    {
        // v3 v0 clip v1 v2
        ps1 = clipLineAtHorizon(cs0, cs1);
        ps2 = clipLineAtHorizon(cs3, cs2);
        return 4;
    }
    // Three points up
    else if(ah0 && ah1 && ah2 && !ah3)
    {
        // v0 v1 v2 clip v3
        ps0 = cs1;
        ps1 = cs2;
        ps2 = clipLineAtHorizon(cs2, cs3);
        ps3 = clipLineAtHorizon(cs0, cs3);
        ps4 = cs0;
        return 5;
    }
    else if(!ah0 && ah1 && ah2 && ah3)
    {
        // v1 v2 v3 clip v0
        ps0 = cs2;
        ps1 = cs3;
        ps2 = clipLineAtHorizon(cs3, cs0);
        ps3 = clipLineAtHorizon(cs1, cs0);
        ps4 = cs1;
        return 5;
    }
    else if(ah0 && !ah1 && ah2 && ah3)
    {
        // v2 v3 v0 clip v1
        ps0 = cs3;
        ps1 = cs0;
        ps2 = clipLineAtHorizon(cs0, cs1);
        ps3 = clipLineAtHorizon(cs2, cs1);
        ps4 = cs2;
        return 5;
    }
    else if(ah0 && ah1 && !ah2 && ah3)
    {
        // v3 v0 v1 clip v2
        ps0 = cs0;
        ps1 = cs1;
        ps2 = clipLineAtHorizon(cs1, cs2);
        ps3 = clipLineAtHorizon(cs3, cs2);
        ps4 = cs3;
        return 5;
    }
    // All points up
    else
    {
        return 4;
    }
}

float calSTDCosIntegration(float3 ps0, float3 ps1, float3 ps2, float3 ps3, float3 ps4, int n)
{
    float cosLobe = 0.0f;

    float tmp = 0.0f;

    if(n==0)
    {
        return 0.0f;
    }

    tmp = acos(dot(ps0, ps1));
    tmp *= dot(normalize(cross(ps0, ps1)), float3(0, 0, 1));
    cosLobe += tmp;

    tmp = acos(dot(ps1, ps2));
    tmp *= dot(normalize(cross(ps1, ps2)), float3(0, 0, 1));
    cosLobe += tmp;

    if(n==3)
    {
        tmp = acos(dot(ps2, ps0));
        tmp *= dot(normalize(cross(ps2, ps0)), float3(0, 0, 1));
        cosLobe += tmp;

        cosLobe = cosLobe / 2.0f / 3.14159f;
        return cosLobe;
    }

    tmp = acos(dot(ps2, ps3));
    tmp *= dot(normalize(cross(ps2, ps3)), float3(0, 0, 1));
    cosLobe += tmp;

    if(n==4)
    {
        tmp = acos(dot(ps3, ps0));
        tmp *= dot(normalize(cross(ps3, ps0)), float3(0, 0, 1));
        cosLobe += tmp;

        cosLobe = cosLobe / 2.0f / 3.14159f;
        return cosLobe;
    }

    tmp = acos(dot(ps3, ps4));
    tmp *= dot(normalize(cross(ps3, ps4)), float3(0, 0, 1));
    cosLobe += tmp;

    // n==5
    tmp = acos(dot(ps4, ps0));
    tmp *= dot(normalize(cross(ps4, ps0)), float3(0, 0, 1));
    cosLobe += tmp;

    cosLobe = cosLobe / 2.0f / 3.14159f;
    return cosLobe;
}

float clipAndCalSTDCosIntegration(float3 ps0, float3 ps1, float3 ps2, float3 ps3)
{
    float3 ps4 = float3(0, 0, 0);
    
    // clip, remove parts under horizon
    int n = 0;
    n = clipUnderHorizon(ps0, ps1, ps2, ps3, ps4);

    // Cal standard cosine lobe integration
    float cosLobe = calSTDCosIntegration(ps0, ps1, ps2, ps3, ps4, n);
    cosLobe = max(-cosLobe, 0.0f);

    return cosLobe;
}

float calIntegration(float3 ps0, float3 ps1, float3 ps2, float3 ps3, float3x3 transMat)
{
    // Transform points & Project onto unit sphere
    ps0 = normalize(mul(ps0, transMat));
    ps1 = normalize(mul(ps1, transMat));
    ps2 = normalize(mul(ps2, transMat));
    ps3 = normalize(mul(ps3, transMat));

    // Cal standard cosine lobe's integration after clip horizon
    return clipAndCalSTDCosIntegration(ps0, ps1, ps2, ps3);
}

float3 calSpecularBRDFWithCos_rect(
    float3 viewDir, float3 normal,
    Texture2D ltcMatTex, Texture2D ltcAmpTex,
    float4 vertDirs[4], float3x3 TBNMat
    )
{
    // load LTC mat && amp
    float3x3 ltcMatInv;
    float ltcAmp;
    loadLTCMatAndAmp(ltcMatInv, ltcAmp,
        viewDir, normal,
        ltcMatTex, ltcAmpTex, bilinearWrap,
        gRoughness
    );

    // Calculate Integration with a LTC transformation.
    float cosLobe = calIntegration(
        vertDirs[0].xyz, vertDirs[1].xyz, 
        vertDirs[2].xyz, vertDirs[3].xyz, 
        mul(TBNMat, ltcMatInv)
    );

    // Multiple LTC lobe's magnitude
    cosLobe *= ltcAmp;

    // Calculate centerDir as an approximation for Fresnel
    float3 centerDir = vertDirs[0].xyz;
    centerDir += vertDirs[1].xyz;
    centerDir += vertDirs[2].xyz;
    centerDir += vertDirs[3].xyz;
    centerDir = normalize(centerDir);

    // Calculate Fresnel reflectance, using Schlick approximation
    float3 fresnel = calFresnelReflectance(normal, centerDir);

    // Multiple Fresnel reflectance
    return cosLobe * fresnel;
}

float3 calDiffuseBRDFWithCos_rect(
    float3 viewDir, float3 normal, 
    float4 vertDirs[4], float3x3 TBNMat,
    float3 ssAlbedo
    )
{
    // Calculate Integration with a TBN transformation, just using Lambertian BRDF
    float cosLobe = calIntegration(
        vertDirs[0].xyz, vertDirs[1].xyz, 
        vertDirs[2].xyz, vertDirs[3].xyz, 
        TBNMat
    );

    // Multiple standard consine's magnitude
    cosLobe *= 2.0f * 3.14159;

    // Calculate centerDir as an approximation for Fresnel
    float3 centerDir = vertDirs[0].xyz;
    centerDir += vertDirs[1].xyz;
    centerDir += vertDirs[2].xyz;
    centerDir += vertDirs[3].xyz;
    centerDir = normalize(centerDir);

    // Calculate Fresnel reflectance, using Schlick approximation
    float3 fresnel = calFresnelReflectance(normal, centerDir);

    // Calculate diffuse brdf, using Lambertian term
    float3 res = 1 - fresnel;
    res *= ssAlbedo / 3.14159;
    res *= cosLobe;

    // Multiple Fresnel reflectance
    return res;
}

void calBRDF_rect(out float3 out_specular, out float3 out_diffuse, 
    float3 viewDir, float3 normal, float4 vertDirs[4], 
    float3 ssAlbedo
    )
{
    // cal TBNMat
    // Note: TBNMat here is different from TBNMat outside
    float3x3 TBNMat = calTBNofVirtualNormal(viewDir, normal);

    // specular
    if (gLTCAmpTexID > 0 && gLTCMatTexID > 0)
    {
        out_specular = calSpecularBRDFWithCos_rect(
            viewDir, normal,
            gTexs[gLTCMatTexID - 1], gTexs[gLTCAmpTexID - 1],
            vertDirs, TBNMat
        );
    }
    else
    {
        // Calculate Integration with a TBN transformation, just using Lambertian BRDF
        float cosLobe = calIntegration(
            vertDirs[0].xyz, vertDirs[1].xyz, 
            vertDirs[2].xyz, vertDirs[3].xyz, 
            TBNMat
        );

        // Multiple LTC lobe's magnitude
        cosLobe *= 2.0f * 3.14159;

        // Calculate centerDir as an approximation for Fresnel
        float3 centerDir = vertDirs[0].xyz;
        centerDir += vertDirs[1].xyz;
        centerDir += vertDirs[2].xyz;
        centerDir += vertDirs[3].xyz;
        centerDir = normalize(centerDir);

        // Calculate Fresnel reflectance, using Schlick approximation
        float3 fresnel = calFresnelReflectance(normal, centerDir);

        // Multiple Fresnel reflectance
        out_specular = cosLobe * fresnel;
    }

    // diffuse
    out_diffuse = calDiffuseBRDFWithCos_rect(
        viewDir, normal,
        vertDirs, TBNMat,
        ssAlbedo
    );
}

#endif//BRDF_RECT_HEADER