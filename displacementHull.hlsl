#include "Header.hlsli"

// �޲�����������
HullConstant CalcHSPatchConstants(
	InputPatch<VertexOut, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HullConstant Output;

	// �ڴ˴���������Լ������
	Output.EdgeTessFactor[0] = 
		Output.EdgeTessFactor[1] = 
		Output.EdgeTessFactor[2] = 
		Output.InsideTessFactor = 8; // ���磬�ɸ�Ϊ���㶯̬�ָ�����

	return Output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
VertexOut main( 
	InputPatch<VertexOut, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	VertexOut Output;

	// �ڴ˴���������Լ������
	Output = ip[i];

	return Output;
}
