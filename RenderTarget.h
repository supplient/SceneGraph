#pragma once
#include "../Common/d3dUtil.h"

class RenderTarget 
{
public:
	RenderTarget(std::wstring name, DXGI_FORMAT format, FLOAT inClearValue[])
		:name(name), format(format)
	{
		for (int i = 0; i < 4; i++)
			clearValue[i] = inClearValue[i];
	}

	void Resize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_RESOURCE_DESC* desc,
		D3D12_RESOURCE_STATES initState) 
	{
		if (format != desc->Format)
			throw "Format not match!";

		// Build Resource
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			desc,
			initState,
			&CD3DX12_CLEAR_VALUE(
				format, clearValue
			),
			IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())
		));
		resource->SetName(name.c_str());

		// Build Descriptor
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		device->CreateRenderTargetView(
			resource.Get(),
			&rtvDesc, rtvCPUHandle
		);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(
			resource.Get(),
			&srvDesc, srvCPUHandle
		);
	}

	std::wstring name;

	DXGI_FORMAT format;
	FLOAT clearValue[4];
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPUHandle;
};
