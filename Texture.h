#pragma once

#include "Common/d3dUtil.h"

// TODO move this to somewhere
const std::wstring TEXTURE_PATH_HEAD = L"Resources/Textures/";

class ResourceTexture
{
public:
	ResourceTexture(const std::string& name)
		: mName(name)
	{
		mID = sIDCount; 
		sIDCount++;
		sIDMap.push_back(this);
	}
	~ResourceTexture() {
		sIDMap[mID] = nullptr;
	}

	UINT GetID()const { return mID; }
	static UINT GetTotalNum() { return sIDCount; }
	static ResourceTexture* FindResourceTextureByID(UINT id) {
		if (id >= sIDMap.size())
			return nullptr;
		return sIDMap[id];
	}

	void SetFilePath(std::wstring filepath) { mFilePath = filepath; }

	void LoadAndCreateSRV(
		Microsoft::WRL::ComPtr<ID3D12Device> device, 
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle
		);

private:
	// Note: We left UINT32_MAX as an invalid ID.
	UINT mID;
	static UINT sIDCount;
	static std::vector<ResourceTexture*> sIDMap;

	std::string mName;

	std::wstring mFilePath;

	Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadHeap = nullptr;
};
