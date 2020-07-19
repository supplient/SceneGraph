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

	static constexpr float MIN_DIST_FACTOR_SQRT = 0.1f;
};

class DirectionLight : public Light {
public:
	DirectionLight(
		DirectX::XMFLOAT3 color, 
		DirectX::XMFLOAT3 dir, 
		DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f }
	):Light(color){
		DirectX::XMVECTOR alpha = DirectX::XMLoadFloat3(&dir);
		DirectX::XMVECTOR beta = DirectX::XMLoadFloat3(&upDir);
		DirectX::XMVECTOR gamma = DirectX::XMVector3Cross(alpha, beta);
		beta = DirectX::XMVector3Cross(gamma, alpha);

		alpha = DirectX::XMVector3Normalize(alpha);
		beta = DirectX::XMVector3Normalize(beta);
		gamma = DirectX::XMVector3Normalize(gamma);

		XMStoreFloat3(&Direction, alpha);
		XMStoreFloat3(&UpDir, beta);
		XMStoreFloat3(&BiDir, gamma);
	}

	struct Content {
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Direction = { 0.0f, 0.0f, 1.0f, 0.0f };
		DirectX::XMFLOAT4X4 ViewMat = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ProjMat = MathHelper::Identity4x4();
		UINT ShadowSRVID = 0;
		DirectX::XMFLOAT3  padding1;
	};
	Content ToContent()const {
		Content content;

		// Reverse & Normalize direction for convenience
		DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&Direction);
		direction = DirectX::XMVectorScale(direction, -1.0f);
		direction = DirectX::XMVector4Normalize(direction);
		DirectX::XMStoreFloat4(&content.Direction, direction);

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

	void SetViewVolumes(std::array<DirectX::XMFLOAT4, 8> viewVolumes) {
		DirectX::XMMATRIX ABCTransMat;
		ABCTransMat.r[0] = DirectX::XMLoadFloat3(&Direction);
		ABCTransMat.r[1] = DirectX::XMLoadFloat3(&UpDir);
		ABCTransMat.r[2] = DirectX::XMLoadFloat3(&BiDir);
		ABCTransMat.r[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMMATRIX ABCMat = DirectX::XMMatrixTranspose(ABCTransMat);

		std::array<DirectX::XMFLOAT3, 8> ABCVerts;
		for (int i = 0; i < 8; i++) {
			DirectX::XMVECTOR vert = DirectX::XMLoadFloat4(&viewVolumes[i]);
			vert = DirectX::XMVector3Transform(vert, ABCMat);
			DirectX::XMStoreFloat3(&ABCVerts[i], vert);
		}

		float xmin, xmax, ymin, ymax, zmin, zmax;
		{
			const auto& initP = ABCVerts[0];
			xmin = xmax = initP.x;
			ymin = ymax = initP.y;
			zmin = zmax = initP.z;
		}
		for (int i = 1; i < 8; i++) {
			const auto& p = ABCVerts[i];
			if (p.x < xmin) xmin = p.x;
			if (p.x > xmax) xmax = p.x;
			if (p.y < ymin) ymin = p.y;
			if (p.y > ymax) ymax = p.y;
			if (p.z < zmin) zmin = p.z;
			if (p.z > zmax) zmax = p.z;
		}

		DirectX::XMVECTOR eyePos = { xmin-NearPlane, (ymin + ymax) / 2, (zmin + zmax) / 2, 1 };
		eyePos = DirectX::XMVector4Transform(eyePos, ABCTransMat);

		DirectX::XMStoreFloat3(&EyePos, eyePos);
		BoxWidth = ymax - ymin;
		BoxHeight = zmax - zmin;
		FarPlane = xmax - xmin + NearPlane;
	}

	DirectX::XMMATRIX CalLightViewMat()const {
		DirectX::XMVECTOR upDir = XMLoadFloat3(&UpDir);
		DirectX::XMVECTOR eyeDir = DirectX::XMLoadFloat3(&Direction);
		DirectX::XMVECTOR eyePos = DirectX::XMLoadFloat3(&EyePos);
		return DirectX::XMMatrixLookToLH(eyePos, eyeDir, upDir);
	}

	DirectX::XMMATRIX CalLightProjMat()const {
		return DirectX::XMMatrixOrthographicLH(
			BoxWidth, BoxHeight,
			NearPlane, FarPlane
		);
	}

	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 UpDir = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 BiDir = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 EyePos = { 0.0f, 0.0f, 0.0f };
	FLOAT BoxWidth = 1.0f;
	FLOAT BoxHeight = 1.0f;
	const FLOAT NearPlane = 1.0f;
	FLOAT FarPlane = 10.0f;

	std::unique_ptr<SingleRenderTarget> ShadowRT = nullptr;
	UINT ShadowSRVID = 0;
	std::unique_ptr<ShadowPassConstants> PassConstants = nullptr;
};

class PointLight : public Light {
public:
	static const UINT RTVNum = 6;

	PointLight(
		DirectX::XMFLOAT3 color, 
		DirectX::XMFLOAT3 pos,
		FLOAT distMin = 0.01f,
		FLOAT distMax = 10.0f
	):Light(color), Position(pos), 
		DistMin(distMin), DistMax(distMax)
	{
	}

	struct Content {
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Pos = { 0.0f, 0.0f, 0.0f, 1.0f };
		UINT ShadowSRVID = 0;
		FLOAT RMin = 0.01f;
		FLOAT R0 = 1.0f;
		FLOAT padding1;
	};
	Content ToContent()const {
		Content content;

		content.Pos = MathHelper::XMFLOAT3TO4(Position, 1.0f);
		content.Color = MathHelper::XMFLOAT3TO4(Color, 1.0f);
		content.ShadowSRVID = ShadowSRVID;
		content.RMin = DistMin;
		content.R0 = DistMax * MIN_DIST_FACTOR_SQRT; // r0 = rmin * sqrt(epsillon)
		return content;
	}

	std::array<DirectX::XMMATRIX, 6> CalLightViewMats()const {
		DirectX::XMVECTOR eyePos = DirectX::XMLoadFloat3(&Position);
		static const std::array<DirectX::XMVECTOR, 6> eyeDirs = {
			DirectX::XMVECTOR{1.0f, 0.0f, 0.0f, 0.0f}, // +x
			DirectX::XMVECTOR{-1.0f, 0.0f, 0.0f, 0.0f}, // -x
			DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 0.0f}, // +y
			DirectX::XMVECTOR{0.0f, -1.0f, 0.0f, 0.0f}, // -y
			DirectX::XMVECTOR{0.0f, 0.0f, 1.0f, 0.0f}, // +z
			DirectX::XMVECTOR{0.0f, 0.0f, -1.0f, 0.0f} // -z
		};
		static const std::array<DirectX::XMVECTOR, 6> upDirs = {
			DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 0.0f}, // +x
			DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 0.0f}, // -x
			DirectX::XMVECTOR{0.0f, 0.0f, -1.0f, 0.0f}, // +y
			DirectX::XMVECTOR{0.0f, 0.0f, 1.0f, 0.0f}, // -y
			DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 0.0f}, // +z
			DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 0.0f} // -z
		};

		std::array<DirectX::XMMATRIX, 6> res;
		for (int i = 0; i < 6; i++)
			res[i] = DirectX::XMMatrixLookToLH(eyePos, eyeDirs[i], upDirs[i]);
		return res;
	}
	DirectX::XMMATRIX CalLightProjMat()const {
		return DirectX::XMMatrixPerspectiveFovLH(
			MathHelper::AngleToRadius(90.0f),
			1.0f,
			DistMin, DistMax
		);
	}
	
	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
	FLOAT DistMin;
	FLOAT DistMax;

	std::unique_ptr<CubeRenderTarget> ShadowRT = nullptr;
	UINT ShadowSRVID = 0;
	std::array<std::unique_ptr<ShadowPassConstants>, 6> PassConstantsArray;
};

class SpotLight : public Light {
public:
	SpotLight(
		DirectX::XMFLOAT3 color,
		DirectX::XMFLOAT3 pos,
		DirectX::XMFLOAT3 dir,
		FLOAT distMin = 0.01f,
		FLOAT distMax = 10.0f
	):Light(color), Position(pos), Direction(dir),
		DistMin(distMin), DistMax(distMax)
	{
	}

	struct Content {
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Pos = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Direction = { 0.0f, 0.0f, 1.0f, 0.0f };
		DirectX::XMFLOAT4X4 ViewMat = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ProjMat = MathHelper::Identity4x4();
		UINT ShadowSRVID = 0;
		FLOAT RMin = 0.01f;
		FLOAT R0 = 1.0f;
		FLOAT padding1;
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
		content.RMin = DistMin;
		content.R0 = DistMax * MIN_DIST_FACTOR_SQRT; // r0 = rmin * sqrt(epsillon)

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
		return DirectX::XMMatrixPerspectiveFovLH(
			MathHelper::AngleToRadius(45.0f),
			1.0f,
			DistMin, DistMax
		);
	}

	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
	FLOAT DistMin;
	FLOAT DistMax;

	std::unique_ptr<SingleRenderTarget> ShadowRT = nullptr;
	UINT ShadowSRVID = 0;
	std::unique_ptr<ShadowPassConstants> PassConstants = nullptr;
};
