#include "Texture.h"
#include "Common/DDSTextureLoader.h"

UINT ResourceTexture::sIDCount = 0;
std::vector<ResourceTexture*> ResourceTexture::sIDMap;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void ResourceTexture::LoadAndCreateSRV(
	ComPtr<ID3D12Device> device, 
	ComPtr<ID3D12GraphicsCommandList> commandList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle)
{
	ThrowIfFailed(CreateDDSTextureFromFile12(
		device.Get(), commandList.Get(),
		mFilePath.data(),
		mResource, mUploadHeap
	));

	D3D12_RESOURCE_DESC resourceDesc = mResource->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = resourceDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(
		mResource.Get(),
		&srvDesc, descHandle
	);
}