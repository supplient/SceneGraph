#pragma once

#include "Common/d3dUtil.h"

// TODO move this to somewhere
const std::wstring TEXTURE_PATH_HEAD = L"Resources/Textures/";

class Texture
{
public:
	Texture(const std::string& name)
		: mName(name)
	{
		mID = sIDCount; 
		sIDCount++;
		sIDMap.push_back(this);
	}
	~Texture() {
		sIDMap[mID] = nullptr;
	}

	std::string GetName()const { return mName; }
	UINT GetID()const { return mID; }
	static UINT GetTotalNum() { return sIDCount; }
	static Texture* FindByID(UINT id) {
		if (id >= sIDMap.size())
			return nullptr;
		return sIDMap[id];
	}
	static Texture* FindByName(std::string name) {
		for (auto item : sIDMap) {
			if (item->GetName() == name)
				return item;
		}
		return nullptr;
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
	static std::vector<Texture*> sIDMap;

	std::string mName;

	std::wstring mFilePath;

	Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadHeap = nullptr;
};
