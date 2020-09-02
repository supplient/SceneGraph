#pragma once
#include "Common/d3dUtil.h"

class UnorderedAccessBuffer
{
public:
	UnorderedAccessBuffer(
		std::wstring name, DXGI_FORMAT viewFormat, FLOAT inClearValue[]
		)
		:name(name), viewFormat(viewFormat)
	{
		for (int i = 0; i < 4; i++)
			clearValue[i] = inClearValue[i];
	}

	void SetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { CPUHandle = handle; }
	void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) { GPUHandle = handle; }
	void SetCPUHandle_CPUHeap(D3D12_CPU_DESCRIPTOR_HANDLE handle) { CPUHandle_CPUHeap = handle; }

	void Resize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_RESOURCE_DESC* desc
		)
	{
		if (viewFormat != desc->Format)
			throw "Format not match!";

		// Build Resource
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&resource)
		));
		resource->SetName(name.c_str());

		// Build Descriptor
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = viewFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		device->CreateUnorderedAccessView(
			resource.Get(), nullptr,
			&uavDesc, CPUHandle
		);
		device->CreateUnorderedAccessView(
			resource.Get(), nullptr,
			&uavDesc, CPUHandle_CPUHeap
		);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() { return CPUHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() { return GPUHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle_CPUHeap() { return CPUHandle_CPUHeap; }

	DXGI_FORMAT GetViewFormat() { return viewFormat; }

	ID3D12Resource* GetResource() { 
		if (!resource)
			return nullptr;
		return resource.Get(); 
	}

	FLOAT* GetClearValue() {
		// TODO not safe
		return clearValue;
	}

private:
	std::wstring name;
	DXGI_FORMAT viewFormat;
	FLOAT clearValue[4];
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle_CPUHeap;
	
};
