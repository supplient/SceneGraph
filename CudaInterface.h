#pragma once
#include <cuda_runtime.h>

#include "ShaderStruct.h"

void ClothSimulation(Vertex* verts, cudaStream_t streamToRun, float time);
