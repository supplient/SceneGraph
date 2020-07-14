#pragma once
#include "Common/d3dUtil.h"
#include "RenderTarget.h"

class ShadowPassConstants;

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
		DirectX::XMFLOAT4X4 ViewMat = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ProjMat = MathHelper::Identity4x4();
		UINT ShadowSRVID = 0;
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
		content.ShadowSRVID = ShadowSRVID;

		DirectX::XMStoreFloat4x4(
			&content.ViewMat,
			DirectX::XMMatrixTranspose(CalLightViewMat())
		);
		DirectX::XMStoreFloat4x4(
			&content.ProjMat,
			DirectX::XMMatrixTranspose(CalLightProjMat())
		);
		return content;
	}

	DirectX::XMMATRIX CalLightViewMat()const {
		DirectX::XMVECTOR upDir = { 0.0f, 1.0f, 0.0f, 0.0f };
		DirectX::XMVECTOR eyePos = DirectX::XMLoadFloat3(&Position);
		DirectX::XMVECTOR eyeDir = DirectX::XMLoadFloat3(&Direction);
		return DirectX::XMMatrixLookToLH(eyePos, eyeDir, upDir);
	}

	DirectX::XMMATRIX CalLightProjMat()const {
		// TODO These nums are just set for test
		// They should be acquired from light's settings
		return DirectX::XMMatrixPerspectiveFovLH(
			MathHelper::AngleToRadius(45.0f),
			1.0f,
			0.01f, 10.0f
		);
	}

	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };

	std::unique_ptr<SingleRenderTarget> ShadowRT = nullptr;
	UINT ShadowSRVID = 0;
	std::unique_ptr<ShadowPassConstants> PassConstants = nullptr;
};
