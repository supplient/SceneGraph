#pragma once
#include "../Common/d3dUtil.h"
#include "Constants.h"

class RenderItem {
public:
	// Mesh concerned
	std::shared_ptr<MeshGeometry> Geo;
	SubmeshGeometry Submesh;
	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology;

	// Shader concerned
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PSO = nullptr;

	// Constants concerned
	std::shared_ptr<ObjectConstants> Consts = nullptr;
};
