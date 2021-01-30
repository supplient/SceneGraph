#include "CudaInterface.h"
#include "SimulationConst.h"
#include "helper_cuda.h"
#include "device_launch_parameters.h"

using namespace sim;

__device__ int CalVertID(int i, int j, bool isFront)
{
	if (isFront)
		return i * n + j;
	else
		return i * n + j + n * n;
}

__global__ void kernel_ClothSimulation(Vertex* verts, float time)
{
	unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int j = blockIdx.y * blockDim.y + threadIdx.y;

	Vertex& vert = verts[CalVertID(i, j, true)];
	vert.pos.z = sin(time);

	verts[CalVertID(i, j, false)].pos.z = vert.pos.z;
}

void ClothSimulation(Vertex* verts, cudaStream_t streamToRun, float time)
{
	auto max = [](int a, int b) {return a > b ? a : b; };

	dim3 block(16, 16, 1);
	dim3 grid(max(n / 16, 1), max(n / 16, 1), 1);

	kernel_ClothSimulation<<<grid, block, 0, streamToRun >>>(
		verts, time
	);

	getLastCudaError("kernel_ClothSimulation execution failed.\n");
}