#include "Mesh.h"

UINT Mesh::sIDCount = 0;
std::vector<Mesh*> Mesh::sIDMap;

using Microsoft::WRL::ComPtr;

void Mesh::UploadBuffer(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
	if (mVertexBufferGPU) // uploaded
		return;

	mVertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		device.Get(), commandList.Get(),
		mVertexBufferCPU->GetBufferPointer(),
		mVertexBufferByteSize,
		mVertexBufferUpload
	);
	mIndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		device.Get(), commandList.Get(),
		mIndexBufferCPU->GetBufferPointer(),
		mIndexBufferByteSize,
		mIndexBufferUpload
	);
}

void Mesh::DisposeUploaders()
{
	mVertexBufferUpload = nullptr;
	mIndexBufferUpload = nullptr;
}

D3D12_VERTEX_BUFFER_VIEW Mesh::GetVertexBufferView()const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = mVertexByteStride;
	vbv.SizeInBytes = mVertexBufferByteSize;
	return vbv;
}

D3D12_INDEX_BUFFER_VIEW Mesh::GetIndexBufferView()const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = mIndexFormat;
	ibv.SizeInBytes = mVertexBufferByteSize;
	return ibv;
}