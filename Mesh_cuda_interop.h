#pragma once

#include <cuda_runtime.h>

#include "Mesh.h"
#include "WindowsSecurityAttributes.h"


class Mesh_cuda_interop : public Mesh
{
public:
	Mesh_cuda_interop(const std::string& name): Mesh(name) {}
	virtual ~Mesh_cuda_interop() {}
	void BuildCudaExtMem(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT32 nodeMask);
	void* GetCudaDevicePtr();

private:
	virtual D3D12_HEAP_FLAGS GetVertexBufferHeapFlags()const override { return D3D12_HEAP_FLAG_SHARED; }

	cudaExternalMemory_t mCudaExtMem = nullptr;
	void* mDevicePtr = nullptr;
};
