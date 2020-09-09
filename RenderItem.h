#pragma once
#include "Common/d3dUtil.h"
#include "Constants.h"

const UINT INVALID_OBJECT_ID = -1;
const UINT INVALID_MATERIAL_ID = -1;

class RenderItem {
public:
	// Mesh concerned
	UINT MeshID;
	UINT SubMeshID;

	// Material concerned
	UINT MaterialID = INVALID_MATERIAL_ID;

	// Shader concerned
	std::string PSO;

	// Object Constants concerned
	UINT ObjectID = INVALID_OBJECT_ID;
};
