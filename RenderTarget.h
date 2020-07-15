#pragma once
#include "Common/d3dUtil.h"

class RenderTarget {
public:
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() = 0;
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle() = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle() = 0;

	virtual DXGI_FORMAT GetColorViewFormat() = 0;
	virtual DXGI_FORMAT GetDepthStencilResourceFormat() = 0;
	virtual DXGI_FORMAT GetDepthStencilViewFormat() = 0;

	virtual ID3D12Resource* GetColorResource() = 0;
	virtual ID3D12Resource* GetDepthStencilResource() = 0;

	virtual FLOAT* GetColorClearValue() = 0;
	virtual float GetDepthClearValue() = 0;
	virtual UINT8 GetStencilClearValue() = 0;
};


class SingleRenderTarget :public RenderTarget
{
public:
	SingleRenderTarget(
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

	void SetSRVGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
		srvGPUHandle = handle;
	}
	void SetSRVCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		srvCPUHandle = handle;
	}
	void SetRTVCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		rtvCPUHandle = handle;
	}
	void SetDSVCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		dsvCPUHandle = handle;
	}

	void Resize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_RESOURCE_DESC* desc,
		D3D12_RESOURCE_DESC* depthStencilDesc=nullptr
		) 
	{
		if (viewFormat != desc->Format)
			throw "Format not match!";
		if (hasDepthStencil && !depthStencilDesc)
			throw "Must specify depth stencil desc";

		// Check MSAA
		if (hasDepthStencil) {
			if (desc->SampleDesc.Count != depthStencilDesc->SampleDesc.Count)
				throw "Color buffer and DepthStencil buffer's MSAA state must be the same.";
		}
		bool useMSAA = desc->SampleDesc.Count > 1;

		// Build Resource
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			desc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&CD3DX12_CLEAR_VALUE(
				viewFormat, clearValue
			),
			IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())
		));
		resource->SetName(name.c_str());

		// Build RTV & SRV Descriptors
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

		// Build Depth Stencil Resource & Descriptor
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

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() override {
		return srvCPUHandle;
	}
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() override {
		return srvGPUHandle;
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle() override {
		return rtvCPUHandle;
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle() override {
		return dsvCPUHandle;
	}

	virtual DXGI_FORMAT GetColorViewFormat() override {
		return viewFormat;
	}
	virtual DXGI_FORMAT GetDepthStencilResourceFormat() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		return dsResourceFormat;
	}
	virtual DXGI_FORMAT GetDepthStencilViewFormat() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		return dsViewFormat;
	}

	virtual ID3D12Resource* GetColorResource() override {
		if (!resource)
			return nullptr;
		return resource.Get();
	}
	virtual ID3D12Resource* GetDepthStencilResource() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		if (!dsResource)
			return nullptr;
		return dsResource.Get();
	}

	virtual FLOAT* GetColorClearValue() override {
		// TODO not safe
		return clearValue;
	}
	virtual float GetDepthClearValue() override {
		return depthClearValue;
	}
	virtual UINT8 GetStencilClearValue() override {
		return stencilClearValue;
	}

private:
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

class CubeRenderTarget :public RenderTarget
{
public:
	CubeRenderTarget(
		std::wstring name, DXGI_FORMAT viewFormat, FLOAT inClearValue[],
		UINT rtvDescriptorSize,
		bool hasDepthStencil=false, 
		std::wstring dsName=L"depth stencil",
		DXGI_FORMAT dsResourceFormat=DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT dsViewFormat=DXGI_FORMAT_D24_UNORM_S8_UINT,
		float depthClearValue=1.0f,
		UINT8 stencilClearValue=0
		)
		:name(name), viewFormat(viewFormat),
		rtvDescriptorSize(rtvDescriptorSize),
		hasDepthStencil(hasDepthStencil), dsName(dsName),
		dsResourceFormat(dsResourceFormat), dsViewFormat(dsViewFormat),
		depthClearValue(depthClearValue), stencilClearValue(stencilClearValue)
	{
		for (int i = 0; i < 4; i++)
			clearValue[i] = inClearValue[i];
	}

	void SetSRVGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
		srvGPUHandle = handle;
	}
	void SetSRVCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		srvCPUHandle = handle;
	}
	void SetRTVCPUHandleStart(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		rtvCPUHandleStart = handle;
	}
	void SetDSVCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		dsvCPUHandle = handle;
	}

	void Resize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_RESOURCE_DESC* desc,
		D3D12_RESOURCE_DESC* depthStencilDesc=nullptr
		) 
	{
		if (viewFormat != desc->Format)
			throw "Format not match!";
		if (hasDepthStencil && !depthStencilDesc)
			throw "Must specify depth stencil desc";

		// Check MSAA
		if (hasDepthStencil) {
			if (desc->SampleDesc.Count != depthStencilDesc->SampleDesc.Count)
				throw "Color buffer and DepthStencil buffer's MSAA state must be the same.";
		}
		bool useMSAA = desc->SampleDesc.Count > 1;

		// Build Resource
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			desc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&CD3DX12_CLEAR_VALUE(
				viewFormat, clearValue
			),
			IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())
		));
		resource->SetName(name.c_str());

		// Build RTV & SRV Descriptors
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = viewFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 1;
		for (int i = 0; i < 6; i++) {
			rtvDesc.Texture2DArray.FirstArraySlice = i;
			device->CreateRenderTargetView(
				resource.Get(),
				&rtvDesc, 
				CD3DX12_CPU_DESCRIPTOR_HANDLE(
					rtvCPUHandleStart,
					i, rtvDescriptorSize
				)
			);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = viewFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(
			resource.Get(),
			&srvDesc, srvCPUHandle
		);

		// Build Depth Stencil Resource & Descriptor
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

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = dsViewFormat;
			dsvDesc.Texture2D.MipSlice = 0;
			device->CreateDepthStencilView(
				dsResource.Get(),
				&dsvDesc,
				dsvCPUHandle
			);
		}
	}

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() override {
		return srvCPUHandle;
	}
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() override {
		return srvGPUHandle;
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle() override {
		return rtvCPUHandleStart;
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle() override {
		return dsvCPUHandle;
	}

	virtual DXGI_FORMAT GetColorViewFormat() override {
		return viewFormat;
	}
	virtual DXGI_FORMAT GetDepthStencilResourceFormat() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		return dsResourceFormat;
	}
	virtual DXGI_FORMAT GetDepthStencilViewFormat() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		return dsViewFormat;
	}

	virtual ID3D12Resource* GetColorResource() override {
		if (!resource)
			return nullptr;
		return resource.Get();
	}
	virtual ID3D12Resource* GetDepthStencilResource() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		if (!dsResource)
			return nullptr;
		return dsResource.Get();
	}

	virtual FLOAT* GetColorClearValue() override {
		// TODO not safe
		return clearValue;
	}
	virtual float GetDepthClearValue() override {
		return depthClearValue;
	}
	virtual UINT8 GetStencilClearValue() override {
		return stencilClearValue;
	}

private:
	UINT rtvDescriptorSize;

	std::wstring name;
	DXGI_FORMAT viewFormat;
	FLOAT clearValue[4];
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandleStart;
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


class SwapChainRenderTarget : public RenderTarget 
{
public:
	SwapChainRenderTarget(
		std::wstring name, 
		Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		DXGI_SWAP_CHAIN_DESC* desc, FLOAT inClearValue[],
		UINT rtvDescriptorSize, UINT srvDescriptorSize,
		bool hasDepthStencil = false,
		std::wstring dsName = L"depth stencil",
		DXGI_FORMAT dsResourceFormat = DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT dsViewFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		float depthClearValue = 1.0f, UINT8 stencilClearValue = 0
		) 
		:name(name), rtvDescriptorSize(rtvDescriptorSize), srvDescriptorSize(srvDescriptorSize),
		hasDepthStencil(hasDepthStencil), dsName(dsName),
		dsResourceFormat(dsResourceFormat), dsViewFormat(dsViewFormat),
		depthClearValue(depthClearValue), stencilClearValue(stencilClearValue)
	{
		nowBufferIndex = 0;

		viewFormat = desc->BufferDesc.Format;
		bufferCount = desc->BufferCount;

		swapChainBuffers.resize(bufferCount, nullptr);

		for (int i = 0; i < 4; i++)
			clearValue[i] = inClearValue[i];

		ThrowIfFailed(dxgiFactory->CreateSwapChain(
			commandQueue.Get(),
			desc,
			swapChain.ReleaseAndGetAddressOf()
		));
	}


	void SetSRVGPUHandleStart(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
		srvGPUHandleStart = handle;
	}
	void SetSRVCPUHandleStart(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		srvCPUHandleStart = handle;
	}
	void SetRTVCPUHandleStart(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
		rtvCPUHandleStart = handle;
	}
	void SetDSVCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle){
		dsvCPUHandle = handle;
	}

	UINT GetSwapChainBufferCount() {
		return bufferCount;
	}

	void Swap() {
		ThrowIfFailed(swapChain->Present(0, 0));
		nowBufferIndex = (nowBufferIndex + 1) % bufferCount;
	}

	void Resize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		UINT width, UINT height,
		D3D12_RESOURCE_DESC* depthStencilDesc = nullptr
		)
	{
		if (hasDepthStencil && !depthStencilDesc)
			throw "Must specify depth stencil desc";

		// Release previous buffers
		for (UINT i = 0; i < bufferCount; i++)
			swapChainBuffers[i].Reset();

		// Resize the swap chain
		ThrowIfFailed(swapChain->ResizeBuffers(
			bufferCount,
			width, height,
			viewFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
		));
		nowBufferIndex = 0;

		// Build swap chain's RTV & SRV
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = viewFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = viewFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvCPUHandleStart);
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvCPUHandleStart);
		for (UINT i = 0; i < bufferCount; i++) {
			ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffers[i].ReleaseAndGetAddressOf())));
			device->CreateRenderTargetView(
				swapChainBuffers[i].Get(), 
				&rtvDesc, rtvHandle
			);
			device->CreateShaderResourceView(
				swapChainBuffers[i].Get(),
				&srvDesc, srvHandle
			);
			rtvHandle.Offset(1, rtvDescriptorSize);
			srvHandle.Offset(1, srvDescriptorSize);
		}

		// Build Depth Stencil Resource & Descriptor
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

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = dsViewFormat;
			dsvDesc.Texture2D.MipSlice = 0;
			device->CreateDepthStencilView(
				dsResource.Get(),
				&dsvDesc,
				dsvCPUHandle
			);
		}
	}

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() override{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			srvCPUHandleStart,
			nowBufferIndex, srvDescriptorSize
		);
	}
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() override{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(
			srvGPUHandleStart,
			nowBufferIndex, srvDescriptorSize
		);
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle() override{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			rtvCPUHandleStart,
			nowBufferIndex, rtvDescriptorSize
		);
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle() override{
		return dsvCPUHandle;
	}

	virtual DXGI_FORMAT GetColorViewFormat() override {
		return viewFormat;
	}
	virtual DXGI_FORMAT GetDepthStencilResourceFormat() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		return dsResourceFormat;
	}
	virtual DXGI_FORMAT GetDepthStencilViewFormat() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		return dsViewFormat;
	}

	virtual ID3D12Resource* GetColorResource() override {
		return swapChainBuffers[nowBufferIndex].Get();
	}
	virtual ID3D12Resource* GetDepthStencilResource() override {
		if (!hasDepthStencil)
			throw "Does not have depth stencil buffer.";
		if (!dsResource)
			return nullptr;
		return dsResource.Get();
	}

	virtual FLOAT* GetColorClearValue() override {
		// TODO not safe
		return clearValue;
	}
	virtual float GetDepthClearValue() override {
		return depthClearValue;
	}
	virtual UINT8 GetStencilClearValue() override {
		return stencilClearValue;
	}


private:
	UINT rtvDescriptorSize;
	UINT srvDescriptorSize;

	std::wstring name;
	DXGI_FORMAT viewFormat;
	FLOAT clearValue[4];

	UINT bufferCount;
	UINT nowBufferIndex;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> swapChainBuffers;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPUHandleStart;

	bool hasDepthStencil;
	std::wstring dsName;
	DXGI_FORMAT dsResourceFormat;
	DXGI_FORMAT dsViewFormat;
	float depthClearValue;
	UINT8 stencilClearValue;
	Microsoft::WRL::ComPtr<ID3D12Resource> dsResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUHandle;
};
