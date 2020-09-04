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
	std::string Material;

	// Shader concerned
	std::string PSO;

	// Object Constants concerned
	std::shared_ptr<ObjectConstants> ObjConsts = nullptr;
	UINT ObjectID = 0;
};
