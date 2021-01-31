#pragma once
#include <cuda_runtime.h>

#include "ShaderStruct.h"

namespace sim {
	void ClothSimulation(Vertex* verts, cudaStream_t streamToRun, float time);
	void InitClothSimulation(cudaStream_t streamToRun);
	void UpdateNormal(Vertex* verts, cudaStream_t streamToRun);
}

