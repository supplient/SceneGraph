#include "Predefine.hlsli"
#include "MultiSampleLoader.hlsli"

cbuffer cbHBAO : register(b0)
{
    float gScaleX;
    float gScaleY;
    float gNearZ;
    float gFarZ;
    uint gClientWidth;
    uint gClientHeight;
    float2 padding1;
};

#ifdef MULTIPLE_SAMPLE
    // TODO
    Texture2DMS<float> depthTex : register(t0);
    Texture2DMS<float> colorTex : register(t1);
#else
    Texture2D depthTex : register(t0);
    Texture2D colorTex : register(t1);
#endif

float3 posS2posE(int2 posS, uint sampleIndex)
{
    // TODO this is inefficient, remove these clamp
    posS.x = clamp(posS.x, 0, gClientWidth);
    posS.y = clamp(posS.y, 0, gClientHeight);

    float depth = LoadFloat4(depthTex, posS, sampleIndex).r;
    float2 uv = (float2) posS / float2(gClientWidth, gClientHeight);
    uv = uv * 2.0f - 1.0f;

    float x = uv.x * gScaleX;
    float y = -uv.y * gScaleY;
    float z = depth * (gFarZ - gNearZ) + gNearZ;
    return float3(x, y, z);
}

float3 minDiff(float3 pl, float3 p, float3 pr)
{
    float3 lv = pl - p;
    float3 rv = p - pr;
    float lvLen = dot(lv, lv);
    float rvLen = dot(rv, rv);
    if(lvLen > rvLen)
        return rv;
    else
        return lv;
}

float3 reconstructNormal(int2 posS, uint sampleIndex)
{
    float3 p = posS2posE(posS, sampleIndex);
    float3 pl = posS2posE(posS - int2(1, 0), sampleIndex);
    float3 pr = posS2posE(posS + int2(1, 0), sampleIndex);
    float3 pu = posS2posE(posS - int2(0, 1), sampleIndex);
    float3 pd = posS2posE(posS + int2(0, 1), sampleIndex);
    float3 normal = cross(minDiff(pl, p, pr), minDiff(pu, p, pd));
    return normalize(normal);
}

static float PI = 3.14156f;
static int R = 3;

float2 screenLen2EyeLen(int2 lenS)
{
    float2 lenE = (float2) lenS / float2(gClientWidth, gClientHeight);
    lenE = lenE * 2.0f; // lenH
    lenE = lenE * float2(gScaleX, gScaleY);
    return lenE;
}

float calIntegrationPart(int2 deltaGeo, float3 normalE, int2 midS, uint sampleIndex)
{
    float3 posE = posS2posE(midS, sampleIndex);
    float RinEye = length(screenLen2EyeLen(R * deltaGeo)); 

    // Cal tangent plane angle
    float3 sampleDirE = normalize(float3(deltaGeo.x, -deltaGeo.y, 0.0f));
    float tanAngle = acos(dot(normalE, sampleDirE)) - PI / 2.0f;

    // Cal horzion angle & distance
    // TODO here we hard-encode R
    int2 p1S = midS + deltaGeo;
    int2 p2S = p1S + deltaGeo;
    int2 p3S = p2S + deltaGeo;

    float3 p1 = posS2posE(p1S, sampleIndex);
    float3 p2 = posS2posE(p2S, sampleIndex);
    float3 p3 = posS2posE(p3S, sampleIndex);

    float3 D1 = p1 - posE;
    float3 D2 = p2 - posE;
    float3 D3 = p3 - posE;

    float alpha1 = atan2(-D1.z, length(D1.xy));
    float alpha2 = atan2(-D2.z, length(D2.xy));
    float alpha3 = atan2(-D3.z, length(D3.xy));

    float horzionAngle = 0.0f;
    float dist = 0.0f;
    if(alpha1 > alpha2 && alpha1 > alpha3 && alpha1 > tanAngle)
    {
        horzionAngle = alpha1;
        dist = distance(p1, posE);
    }
    else if(alpha2 > alpha1 && alpha2 > alpha3 && alpha2 > tanAngle)
    {
        horzionAngle = alpha2;
        dist = distance(p2, posE);
    }
    else if(alpha3 > alpha1 && alpha3 > alpha2 && alpha3 > tanAngle)
    {
        horzionAngle = alpha3;
        dist = distance(p3, posE);
    }
    else if(tanAngle > alpha1 && tanAngle > alpha2 && tanAngle > alpha3)
    {
        horzionAngle = tanAngle;
        dist = RinEye;
    }

    // Cal attenuation factor
    float distFactor = 1 - dist/RinEye/3.0f;
    float distAtte = max(0.1f, distFactor);

    // Cal part of the integration
    float res = distAtte * (sin(horzionAngle) - sin(tanAngle));
    
    return res;
}

float calAmbientOcclusion(float3 normalE, int2 posS, uint sampleIndex)
{
    float integration = 0;

    integration += calIntegrationPart(int2(-1, 0), normalE, posS, sampleIndex);
    integration += calIntegrationPart(int2(1, 0), normalE, posS, sampleIndex);
    integration += calIntegrationPart(int2(0, -1), normalE, posS, sampleIndex);
    integration += calIntegrationPart(int2(0, 1), normalE, posS, sampleIndex);
    integration /= 4.0f;
    
    float k = (1.0f - integration / 2.0f / PI) / PI;
    return k;
}

#define HBAO_DEBUG 1

float4 main(float4 posH : SV_POSITION, uint sampleIndex : SV_SampleIndex) : SV_TARGET
{
    int2 posS;
    posS = floor(posH.xy);
    float depth = LoadFloat4(depthTex, posS, sampleIndex).r;
    if(depth == 1.0f)
        clip(-1);

    float3 lightColor = LoadFloat4(colorTex, posS, sampleIndex).xyz;
#if HBAO_DEBUG
    float3 ambientLight = float3(1.0f, 1.0f, 1.0f);
#else
    float3 ambientLight = float3(0.2f, 0.2f, 0.2f);
#endif
    if (posS.x < R || posS.x > int(gClientWidth) - R - 1 || posS.y < R || posS.y > int(gClientHeight) - R - 1)
#if HBAO_DEBUG
        return float4(ambientLight, 1.0f);
#else
        return float4(ambientLight + lightColor, 1.0f);
#endif

    float3 normalE = reconstructNormal(posS, sampleIndex);

    float aoFactor = calAmbientOcclusion(normalE, posS, sampleIndex);
    float3 ambientColor = PI * ambientLight * aoFactor;
    
#if HBAO_DEBUG
    return float4(ambientColor, 1.0f);
#else
    return float4(ambientColor + lightColor, 1.0f);
#endif
}