#ifndef MULTI_SAMPLE_LOADER_H
#define MULTI_SAMPLE_LOADER_H

// #define MULTIPLE_SAMPLE 4

#ifdef MULTIPLE_SAMPLE

float4 LoadFloat4(const Texture2DMS<float4> tex, int2 pos, uint sampleIndex)
{
    return tex.Load(pos, sampleIndex);
}

float4 LoadFloat(const Texture2DMS<float> tex, int2 pos, uint sampleIndex)
{
    return tex.Load(pos, sampleIndex);
}

#else

float4 LoadFloat4(Texture2D tex, int2 pos, uint sampleIndex)
{
    return tex.Load(int3(pos, 0));
}

float4 LoadFloat(Texture2D tex, int2 pos, uint sampleIndex)
{
    return tex.Load(int3(pos, 0));
}

#endif// MULTIPLE_SAMPLE

#endif// MULTI_SAMPLE_LOADER_H
