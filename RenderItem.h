#pragma once
#include "Common/d3dUtil.h"
#include "Constants.h"
#include "Object.h"

class RenderItem {
public:
	// Mesh concerned
	std::shared_ptr<MeshGeometry> Geo;
	SubmeshGeometry Submesh;
	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology;

	// Material concerned
	std::shared_ptr<MaterialConstants> MtlConsts = nullptr;

	// Shader concerned
	std::string PSO;

	// Object Constants concerned
	std::shared_ptr<ObjectConstants> ObjConsts = nullptr;
};
