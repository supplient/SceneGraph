#pragma once
#include "Common/d3dUtil.h"
#include "Constants.h"

const UINT INVALID_OBJECT_ID = UINT32_MAX;

class RenderItem {
public:
	// Mesh concerned
	std::string MeshPool;
	std::string SubMesh;
	D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology;

	// Material concerned
	std::string Material;

	// Shader concerned
	std::string PSO;

	// Object Constants concerned
	UINT ObjectID = INVALID_OBJECT_ID;
};
