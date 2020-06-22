#pragma once
#include "../Common/d3dUtil.h"
#include "Constants.h"

class RenderItem {
public:
	// Mesh concerned
	std::shared_ptr<MeshGeometry> Geo;
	SubmeshGeometry Submesh;
	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology;

	// Material concerned
	std::shared_ptr<MaterialConstants> MtlConsts = nullptr;

	// Shader concerned
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PSO = nullptr;

	// Object Constants concerned
	std::shared_ptr<ObjectConstants> ObjConsts = nullptr;
};
