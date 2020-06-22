#pragma once
#include "../Common/d3dUtil.h"

class Light {
public:
	Light(DirectX::XMFLOAT3 color)
		:Color(color) {
	}
	DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };
};

class DirectionLight : public Light {
public:
	DirectionLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 dir)
		:Light(color), Direction(dir) {
	}

	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
};

class PointLight : public Light {
public:
	PointLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 pos)
		:Light(color), Position(pos) {
	}

	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
};

class SpotLight : public Light {
public:
	SpotLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir)
		:Light(color), Position(pos), Direction(dir) {
	}

	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
};
