#pragma once

#include "Common/d3dUtil.h"

const std::wstring TEXTURE_PATH_HEAD = L"Resources/Textures/";

class Texture
{
public:
	UINT32 ID=0;
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
};

class ResourceTexture: public Texture
{
public:
	ResourceTexture(std::wstring filename):Filename(filename) {}

	std::wstring Filename;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};
