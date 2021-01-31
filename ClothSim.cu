#include <DirectXMath.h>

#include "helper_math.h"
#include "helper_cuda.h"
#include "device_launch_parameters.h"

#include "SimulationConst.h"
#include "CudaInterface.h"

using namespace sim;
using DirectX::XMFLOAT3;


namespace sim {
	__managed__ XMFLOAT3* x = nullptr;
	__managed__ size_t x_pitch;
	__managed__ XMFLOAT3* v = nullptr;
	__managed__ size_t v_pitch;
	__managed__ float* w = nullptr;
	__managed__ size_t w_pitch;

	__managed__ XMFLOAT3* x_next = nullptr;
	__managed__ XMFLOAT3* v_next = nullptr;

	__constant__ float3 windForce = { 2.4f, 0.0f, 2.4f };
	constexpr float k_tension = 25.0f;
	constexpr float d_tension = 2.0f;
	constexpr float k_cut = k_tension/10.0f;
	constexpr float d_cut = d_tension/10.0f;
	constexpr float k_bend = k_cut/10.0f;
	constexpr float d_bend = d_cut/10.0f;
	constexpr float k_drag = 1.0f;
	constexpr float k_rise = 0.5f;

	auto max = [](int a, int b) {return a > b ? a : b; };
	dim3 block(16, 16, 1);
	dim3 grid(max((n-1)/16+1, 1), max((n-1)/16+1, 1), 1);
}


__device__ int CalVertID(int i, int j, bool isFront)
{
	if (isFront)
		return i * n + j;
	else
		return i * n + j + n * n;
}

__device__ unsigned int GetThreadi()
{
	return blockIdx.x * blockDim.x + threadIdx.x;
}

__device__ unsigned int GetThreadj()
{
	return blockIdx.y * blockDim.y + threadIdx.y;
}

__device__ void* GetTwoDimArray(void* arr, unsigned int i, unsigned int j, size_t pitch, size_t eleSize)
{
	return (void*)((char*)arr + i * pitch + j * eleSize);
}

__device__ float3& GetX(int i, int j)
{
	return *(float3*)GetTwoDimArray(x, i, j, x_pitch, sizeof(float3));
}

__device__ float3& GetV(int i, int j)
{
	return *(float3*)GetTwoDimArray(v, i, j, v_pitch, sizeof(float3));
}

__device__ float3& GetXNext(int i, int j)
{
	return *(float3*)GetTwoDimArray(x_next, i, j, x_pitch, sizeof(float3));
}

__device__ float3& GetVNext(int i, int j)
{
	return *(float3*)GetTwoDimArray(v_next, i, j, v_pitch, sizeof(float3));
}

__device__ float& GetW(int i, int j)
{
	return *(float*)GetTwoDimArray(w, i, j, w_pitch, sizeof(float));
}

__device__ float3 CalForceFromNeighbor(int i, int j, int a, int b, float k, float d, float l0)
{
	float3 F = { 0, 0, 0 };
	if (a >= 0 && a < n && b >= 0 && b < n) {
		float3 src = GetX(a, b);
		float3 dst = GetX(i, j);
		float3 xDelta = src - dst;
		float xDeltaLen = length(xDelta);
		xDelta = normalize(xDelta);
		float3 vSrc = GetV(a, b);
		float3 vDst = GetV(i, j);
		float3 vDelta = vSrc - vDst;
		float lenDiff = xDeltaLen - l0;

		F += k * lenDiff * xDelta; // 弹簧拉力
		F += d * dot(xDelta, vDelta) * xDelta; // 弹簧阻尼
	}
	return F;
}

__device__ float3 CalWindForceFromNeighbor(int i, int j, int a, int b)
{
	float3 F = { 0.0f, 0.0f, 0.0f };
	if (a >= 0 && a < n - 1 && b >= 0 && b < n - 1) {
		float3 vLeftUp		= GetV(a  , b  );
		float3 vLeftDown	= GetV(a+1, b  );
		float3 vRightUp		= GetV(a  , b+1);
		float3 vRightDown	= GetV(a+1, b+1);
		float3 v_quad = vLeftUp + vLeftDown + vRightUp + vRightDown;
		v_quad /= 4.0f;
		float3 v_rel = v_quad - windForce;
		float3 norm = cross(GetX(a, b) - GetX(a + 1, b), GetX(a, b) - GetX(a, b + 1));
		norm = normalize(norm);
		float S = length(cross(GetX(a, b) - GetX(a + 1, b), GetX(a, b) - GetX(a, b + 1)));
		S += length(cross(GetX(a + 1, b + 1) - GetX(a + 1, b), GetX(a + 1, b + 1) - GetX(a, b + 1)));
		S /= 2.0f;
		S *= abs(dot(norm, v_rel));

		F += -0.25 * S * k_drag * v_rel;
        F += - 0.25 * S * k_rise * cross(v_rel, normalize(cross(norm, v_rel)));
	}
	return F;
}

__global__ void kernel_ClothSimulation(Vertex* verts, float time)
{
	int i = GetThreadi();
	int j = GetThreadj();

	if (i < n && j < n) {
		// Cal Force
		float3 F = { 0.0f, 0.0f, 0.0f };
		{
			// 张力
			F += CalForceFromNeighbor(i, j, i - 1, j, k_tension, d_tension, lConst);
			F += CalForceFromNeighbor(i, j, i + 1, j, k_tension, d_tension, lConst);
			F += CalForceFromNeighbor(i, j, i, j-1, k_tension, d_tension, lConst);
			F += CalForceFromNeighbor(i, j, i, j+1, k_tension, d_tension, lConst);
			// 平面内剪切力
			F += CalForceFromNeighbor(i, j, i-1, j-1, k_cut, d_cut, lCutConst);
			F += CalForceFromNeighbor(i, j, i+1, j-1, k_cut, d_cut, lCutConst);
			F += CalForceFromNeighbor(i, j, i-1, j+1, k_cut, d_cut, lCutConst);
			F += CalForceFromNeighbor(i, j, i+1, j+1, k_cut, d_cut, lCutConst);
			// 平面外弯曲力
			F += CalForceFromNeighbor(i, j, i-2, j, k_bend, d_bend, lBendConst);
			F += CalForceFromNeighbor(i, j, i+2, j, k_bend, d_bend, lBendConst);
			F += CalForceFromNeighbor(i, j, i, j-2, k_bend, d_bend, lBendConst);
			F += CalForceFromNeighbor(i, j, i, j+2, k_bend, d_bend, lBendConst);
			// 风力
			F += CalWindForceFromNeighbor(i, j, i - 1, j - 1);
			F += CalWindForceFromNeighbor(i, j, i-1, j  );
			F += CalWindForceFromNeighbor(i, j, i  , j-1);
			F += CalWindForceFromNeighbor(i, j, i  , j  );
		}

		// Cal v_next & x_next
		{
			float3& v_next_ele = GetVNext(i, j);
			float& w_ele = GetW(i, j);
			v_next_ele = GetV(i, j) + w_ele * F * timestep;

			float3& x_next_ele = GetXNext(i, j);
			x_next_ele = GetX(i, j) + v_next_ele * timestep;
		}

		// Switch now and next
		{
			GetX(i, j) = GetXNext(i, j);
			GetV(i, j) = GetVNext(i, j);
		}

		// Update geometry's actual vertex position
		{
			// Just a copy.
			float3& x_ele = GetX(i, j);
			Vertex& vert = verts[CalVertID(i, j, true)];
			vert.pos.x = x_ele.x;
			vert.pos.y = x_ele.y;
			vert.pos.z = x_ele.z;

			// Not forget the back face
			verts[CalVertID(i, j, false)].pos.x = vert.pos.x;
			verts[CalVertID(i, j, false)].pos.y = vert.pos.y;
			verts[CalVertID(i, j, false)].pos.z = vert.pos.z;
		}
	}
}

__device__ void CalNormalWithNeighbor(
	float3& out, uint& count,
	int i, int j,
	int ui, int uj,
	int vi, int vj
)
{
	if (
		ui >= 0 && ui < n && uj >= 0 && uj < n &&
		vi >= 0 && vi < n && vj >= 0 && vj < n
		) {
		out += normalize(cross(
			GetX(ui, uj) - GetX(i, j),
			GetX(vi, vj) - GetX(i, j)
		));
		count += 1;
	}
}

__global__ void kernel_UpdateNormal(Vertex* verts)
{
	int i = GetThreadi();
	int j = GetThreadj();

	if (i < n && j < n) {
		float3 norm = { 0.0f, 0.0f, 0.0f };
		uint count = 0;
		CalNormalWithNeighbor(norm, count, i, j, i - 1, j, i, j + 1);
		CalNormalWithNeighbor(norm, count, i, j, i, j + 1, i + 1, j);
		CalNormalWithNeighbor(norm, count, i, j, i + 1, j, i, j - 1);
		CalNormalWithNeighbor(norm, count, i, j, i, j - 1, i - 1, j);
		norm /= float(count);

		Vertex& vert = verts[CalVertID(i, j, true)];
		vert.normal.x = norm.x;
		vert.normal.y = norm.y;
		vert.normal.z = norm.z;

		// Not forget the back face
		Vertex& backVert = verts[CalVertID(i, j, false)];
		backVert.normal.x = -norm.x;
		backVert.normal.y = -norm.y;
		backVert.normal.z = -norm.z;
	}
}

__global__ void kernel_Init()
{
	auto i = GetThreadi();
	auto j = GetThreadj();

	if (i < n && j < n) {
		float3& x_ele = *(float3*)GetTwoDimArray(x, i, j, x_pitch, sizeof(float3));
		x_ele.x = (i - (n - 1) / 2.0f) * lConst;
		x_ele.y = (j - (n - 1) / 2.0f) * lConst;
		x_ele.z = 0.0f;

		float3& v_ele = *(float3*)GetTwoDimArray(v, i, j, v_pitch, sizeof(float3));
		v_ele.x = 0;
		v_ele.y = 0;
		v_ele.z = 0;

		float& w_ele = *(float*)GetTwoDimArray(w, i, j, w_pitch, sizeof(float));
		if (i == 0)
			w_ele = 0;
		else
			w_ele = 1 / m;
	}
}

namespace sim {
	void InitClothSimulation(cudaStream_t streamToRun)
	{
		// Alloc device memory
		cudaMallocPitch(&x, &x_pitch, n * sizeof(XMFLOAT3), n);
		cudaMallocPitch(&v, &v_pitch, n * sizeof(XMFLOAT3), n);
		cudaMallocPitch(&w, &w_pitch, n * sizeof(float), n);

		cudaMallocPitch(&x_next, &x_pitch, n * sizeof(XMFLOAT3), n);
		cudaMallocPitch(&v_next, &v_pitch, n * sizeof(XMFLOAT3), n);

		// Initialize
		kernel_Init<<<grid, block, 0, streamToRun >>>();

		getLastCudaError("kernel_Init execution failed.\n");
	}

	void ClothSimulation(Vertex* verts, cudaStream_t streamToRun, float time)
	{

		kernel_ClothSimulation<<<grid, block, 0, streamToRun >>>(
			verts, time
		);

		getLastCudaError("kernel_ClothSimulation execution failed.\n");
	}

	void UpdateNormal(Vertex* verts, cudaStream_t streamToRun)
	{
		kernel_UpdateNormal<<<grid, block, 0, streamToRun >>> (
			verts
		);
		getLastCudaError("kernel_UpdateNormal execution failed.\n");
	}

	void FreeClothSimulation(cudaStream_t streamToRun)
	{
		// TODO
	}
}