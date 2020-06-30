#pragma once
#include "../Common/d3dUtil.h"

class RenderTarget 
{
public:
	RenderTarget(
		std::wstring name, DXGI_FORMAT viewFormat, FLOAT inClearValue[],
		bool hasDepthStencil=false, 
		std::wstring dsName=L"depth stencil",
		DXGI_FORMAT dsResourceFormat=DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT dsViewFormat=DXGI_FORMAT_D24_UNORM_S8_UINT,
		float depthClearValue=1.0f,
		UINT8 stencilClearValue=0
		)
		:name(name), viewFormat(viewFormat),
		hasDepthStencil(hasDepthStencil), dsName(dsName),
		dsResourceFormat(dsResourceFormat), dsViewFormat(dsViewFormat),
		depthClearValue(depthClearValue), stencilClearValue(stencilClearValue)
	{
		for (int i = 0; i < 4; i++)
			clearValue[i] = inClearValue[i];
	}

	void Resize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_RESOURCE_DESC* desc,
		D3D12_RESOURCE_STATES initState,
		D3D12_RESOURCE_DESC* depthStencilDesc=nullptr
		) 
	{
		if (viewFormat != desc->Format)
			throw "Format not match!";
		if (hasDepthStencil && !depthStencilDesc)
			throw "Must specify depth stencil desc";

		// Build Resource
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			desc,
			initState,
			&CD3DX12_CLEAR_VALUE(
				viewFormat, clearValue
			),
			IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())
		));
		resource->SetName(name.c_str());

		// Build Depth Stencil Resource
		if (hasDepthStencil) {
			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				depthStencilDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&CD3DX12_CLEAR_VALUE(
					dsViewFormat,
					depthClearValue,
					stencilClearValue
				),
				IID_PPV_ARGS(dsResource.ReleaseAndGetAddressOf())
			));
			dsResource->SetName(dsName.c_str());
		}

		// Check MSAA
		if (hasDepthStencil) {
			if (desc->SampleDesc.Count != depthStencilDesc->SampleDesc.Count)
				throw "Color buffer and DepthStencil buffer's MSAA state must be the same.";
		}
		bool useMSAA = desc->SampleDesc.Count > 1;

		// Build Descriptor
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = viewFormat;
		rtvDesc.ViewDimension = useMSAA ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		device->CreateRenderTargetView(
			resource.Get(),
			&rtvDesc, rtvCPUHandle
		);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = viewFormat;
		srvDesc.ViewDimension = useMSAA ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(
			resource.Get(),
			&srvDesc, srvCPUHandle
		);

		if (hasDepthStencil) {
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = useMSAA ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = dsViewFormat;
			dsvDesc.Texture2D.MipSlice = 0;
			device->CreateDepthStencilView(
				dsResource.Get(),
				&dsvDesc,
				dsvCPUHandle
			);
		}
	}

	std::wstring name;
	DXGI_FORMAT viewFormat;
	FLOAT clearValue[4];
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPUHandle;

	bool hasDepthStencil;
	std::wstring dsName;
	DXGI_FORMAT dsResourceFormat;
	DXGI_FORMAT dsViewFormat;
	float depthClearValue;
	UINT8 stencilClearValue;
	Microsoft::WRL::ComPtr<ID3D12Resource> dsResource = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUHandle;
};
