#include "Mesh_cuda_interop.h"

#include "helper_cuda.h"
#include "Common/d3dUtil.h"


using namespace DirectX;
using Microsoft::WRL::ComPtr;


void Mesh_cuda_interop::BuildCudaExtMem(ComPtr<ID3D12Device> device, UINT32 nodeMask)
{
	// Get vertex buffer's NT handle
	HANDLE sharedHandle;
	WindowsSecurityAttributes windowsSecurityAttributes;
	LPCWSTR name = NULL;
	ThrowIfFailed(device->CreateSharedHandle(
		mVertexBufferGPU.Get(), 
		&windowsSecurityAttributes, 
		GENERIC_ALL, 
		name, 
		&sharedHandle)
	);

	// Get buffer's actual size and alignment
	D3D12_RESOURCE_ALLOCATION_INFO d3d12ResourceAllocationInfo;
	d3d12ResourceAllocationInfo = device->GetResourceAllocationInfo(
		nodeMask, 
		1, 
		&CD3DX12_RESOURCE_DESC::Buffer(mVertexBufferByteSize)
	);
	size_t actualSize = d3d12ResourceAllocationInfo.SizeInBytes;
	size_t alignment  = d3d12ResourceAllocationInfo.Alignment;

	// Make external memory handle's descriptor
	cudaExternalMemoryHandleDesc externalMemoryHandleDesc;
	memset(&externalMemoryHandleDesc, 0, sizeof(externalMemoryHandleDesc));

	externalMemoryHandleDesc.type = cudaExternalMemoryHandleTypeD3D12Resource;
	externalMemoryHandleDesc.handle.win32.handle = sharedHandle;
	externalMemoryHandleDesc.size = actualSize;
	externalMemoryHandleDesc.flags = cudaExternalMemoryDedicated;

	// Import the external memory handle from the NT handle
	checkCudaErrors(cudaImportExternalMemory(&mCudaExtMem, &externalMemoryHandleDesc));
}

void* Mesh_cuda_interop::GetCudaDevicePtr() {
	if (mDevicePtr)
		return mDevicePtr;
	cudaExternalMemoryBufferDesc externalMemoryBufferDesc;
	memset(&externalMemoryBufferDesc, 0, sizeof(externalMemoryBufferDesc));
	externalMemoryBufferDesc.offset = 0;
	externalMemoryBufferDesc.size   = mVertexBufferByteSize;
	externalMemoryBufferDesc.flags  = 0;

	checkCudaErrors(cudaExternalMemoryGetMappedBuffer(&mDevicePtr, mCudaExtMem, &externalMemoryBufferDesc));
	return mDevicePtr;
}
