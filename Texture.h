#pragma once

#include "Common/d3dUtil.h"

const std::wstring TEXTURE_PATH_HEAD = L"Resources/Textures/";

class Texture
{
public:
	Texture(std::wstring filename):Filename(filename) {}

	std::wstring Filename;
	UINT32 ID=0;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};
