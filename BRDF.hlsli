#ifndef BRDF_HEADER
#define BRDF_HEADER

#include "predefine.hlsli"

#include "BRDF_punctual.hlsli"
#if RECT_LIGHT_ON
    #include "BRDF_rect.hlsli"
#endif

#endif//BRDF_HEADER