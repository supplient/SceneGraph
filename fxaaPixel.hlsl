#include "Fxaa3_11.hlsli"

cbuffer cbFxaa : register(b0)
{
    float gFxaaQualitySubpix;//1: pack count
    float gFxaaQualityEdgeThreshold;//2
    float2 gFxaaQualityRcpFrame;//4
    float4 gFxaaConsoleRcpFrameOpt;//4
    float4 gFxaaConsoleRcpFrameOpt2;//4
    float4 gFxaaConsole360RcpFrameOpt2;//4
    float gFxaaQualityEdgeThresholdMin;//1
    float gFxaaConsoleEdgeSharpness;//2
    float gFxaaConsoleEdgeThreshold;//3
    float gFxaaConsoleEdgeThresholdMin;//4
    float4 gFxaaConsole360ConstDir;//4
};

Texture2D tex : register(t0);
SamplerState bilinearWrap : register(s0);

float4 main(float4 posH : SV_POSITION) : SV_TARGET
{
    float4 consolePos;
    consolePos.xy = floor(posH.xy);
    consolePos.zw = ceil(posH.xy);

    FxaaTex fxaaTex;
    fxaaTex.smpl = bilinearWrap;
    fxaaTex.tex = tex;
    
    return FxaaPixelShader(
        posH.xy,
        consolePos,
        fxaaTex, fxaaTex, fxaaTex,
        gFxaaQualityRcpFrame,
        gFxaaConsoleRcpFrameOpt,
        gFxaaConsoleRcpFrameOpt2,
        gFxaaConsole360RcpFrameOpt2,
        gFxaaQualitySubpix,
        gFxaaQualityEdgeThreshold,
        gFxaaQualityEdgeThresholdMin,
        gFxaaConsoleEdgeSharpness,
        gFxaaConsoleEdgeThreshold,
        gFxaaConsoleEdgeThresholdMin,
        gFxaaConsole360ConstDir
    );
}