#pragma once
#include "Common/d3dUtil.h"

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

	struct Content {
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Direction = { 0.0f, 0.0f, 1.0f, 0.0f };
	};
	Content ToContent()const {
		Content content;

		// Reverse & Normalize direction for convenience
		DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&Direction);
		direction = DirectX::XMVectorScale(direction, -1.0f);
		direction = DirectX::XMVector4Normalize(direction);
		DirectX::XMStoreFloat4(&content.Direction, direction);

		content.Color = MathHelper::XMFLOAT3TO4(Color, 1.0f);
		return content;
	}

	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
};

class PointLight : public Light {
public:
	PointLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 pos)
		:Light(color), Position(pos) {
	}

	struct Content {
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Pos = { 0.0f, 0.0f, 0.0f, 1.0f };
	};
	Content ToContent()const {
		Content content;

		content.Pos = MathHelper::XMFLOAT3TO4(Position, 1.0f);
		content.Color = MathHelper::XMFLOAT3TO4(Color, 1.0f);
		return content;
	}

	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
};

class SpotLight : public Light {
public:
	SpotLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir)
		:Light(color), Position(pos), Direction(dir) {
	}

	struct Content {
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Pos = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Direction = { 0.0f, 0.0f, 1.0f, 0.0f };
	};
	Content ToContent()const {
		Content content;

		// Reverse & Normalize direction for convenience
		DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&Direction);
		direction = DirectX::XMVectorScale(direction, -1.0f);
		direction = DirectX::XMVector4Normalize(direction);
		DirectX::XMStoreFloat4(&content.Direction, direction);

		content.Pos = MathHelper::XMFLOAT3TO4(Position, 1.0f);
		content.Color = MathHelper::XMFLOAT3TO4(Color, 1.0f);
		return content;
	}

	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
};
