#include "Header.hlsli"

// 修补程序常量函数
HullConstant CalcHSPatchConstants(
	InputPatch<VertexOut, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HullConstant Output;

	// 在此处插入代码以计算输出
	Output.EdgeTessFactor[0] = 
		Output.EdgeTessFactor[1] = 
		Output.EdgeTessFactor[2] = 
		Output.InsideTessFactor = 8; // 例如，可改为计算动态分割因子

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

	// 在此处插入代码以计算输出
	Output = ip[i];

	return Output;
}
