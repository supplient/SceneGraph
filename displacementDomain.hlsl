#include "Header.hlsli"

[domain("tri")]
VertexOut main(
	HullConstant input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<VertexOut, NUM_CONTROL_POINTS> patch)
{
	VertexOut Output;

    Output.normalW = normalize(patch[0].normalW * domain.x + patch[1].normalW * domain.y + patch[2].normalW * domain.z);
    Output.tangentW = normalize(patch[0].tangentW * domain.x + patch[1].tangentW * domain.y + patch[2].tangentW * domain.z);
    Output.tex = patch[0].tex * domain.x + patch[1].tex * domain.y + patch[2].tex * domain.z;
    
    Output.posW = patch[0].posW * domain.x + patch[1].posW * domain.y + patch[2].posW * domain.z;
#if TEXTURE_NUM > 0
    if (gDispTexID > 0)
    {
        float offsetHeight = gTexs[gDispTexID - 1].SampleLevel(bilinearWrap, Output.tex, 0.0f).x * gDispHeightScale;
        float4 offsetVec = offsetHeight * Output.normalW;
        Output.posW -= offsetVec;
    }
#endif

    Output.posH = mul(mul(Output.posW, gViewMat), gProjMat);

	return Output;
}
