#pragma once
#include "Common/d3dUtil.h"

class StaticDescriptorHeap {
public:
	StaticDescriptorHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		const D3D12_DESCRIPTOR_HEAP_DESC& desc
		) 
		:heapDesc(desc)
	{
		ThrowIfFailed(device->CreateDescriptorHeap(
			&desc,
			IID_PPV_ARGS(&heap)
		));
		descCount = 0;
		descSize = device->GetDescriptorHandleIncrementSize(desc.Type);
	}

	UINT Alloc(UINT num=1) {
		if (descCount+num > heapDesc.NumDescriptors)
			throw "No enough space in DescriptorHeap";
		UINT startIndex = descCount;
		descCount += num;
		return startIndex;
	}

	auto GetCPUHandle(UINT index)const {
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			heap->GetCPUDescriptorHandleForHeapStart(),
			descSize * index
		);
	}

	auto GetGPUHandle(UINT index)const {
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(
			heap->GetGPUDescriptorHandleForHeapStart(),
			descSize * index
		);
	}

	UINT GetDescriptorIncrementSize()const {
		return descSize;
	}

	ID3D12DescriptorHeap* GetHeap()const {
		return heap.Get();
	}

private:
	UINT descCount;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	UINT descSize;
};
