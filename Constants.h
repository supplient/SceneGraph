#pragma once
#include "../Common/d3dUtil.h"

constexpr unsigned int MAX_LIGHT_NUM = 16;

class ObjectConstants {
public:
	ObjectConstants() {
		mID = sIDCount;
		sIDCount++;
	}
	UINT getID()const { return mID; }
	static UINT getTotalNum() { return sIDCount; }

	struct Content {
		DirectX::XMFLOAT4X4 ModelMat = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 NormalModelMat = MathHelper::Identity4x4();
	} content;

private:
	UINT mID;
	static UINT sIDCount;
};

class MaterialConstants {
public:
	MaterialConstants() {
		mID = sIDCount;
		sIDCount++;
	}
	UINT getID()const { return mID; }
	static UINT getTotalNum() { return sIDCount; }

	struct Content {
		DirectX::XMFLOAT4 Diffuse = { 1.0, 1.0, 1.0, 1.0 };
	} content;

private:
	UINT mID;
	static UINT sIDCount;
};

class LightConstants {
public:
	LightConstants() {
		mID = sIDCount;
		sIDCount++;
	}
	UINT getID()const { return mID; }
	static UINT getTotalNum() { return sIDCount; }

	struct Content {
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Pos = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Direction = { 0.0f, 0.0f, 1.0f, 0.0f };
	} content;

private:
	UINT mID;
	static UINT sIDCount;
};

class PassConstants {
public:
	PassConstants() {
		mID = sIDCount;
		sIDCount++;
	}
	UINT getID()const { return mID; }
	static UINT getTotalNum() { return sIDCount; }

	struct Content {
		DirectX::XMFLOAT4X4 ViewMat = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ProjMat = MathHelper::Identity4x4();
		DirectX::XMUINT4 LightPerTypeNum = { 0, 0, 0, 0 };
		LightConstants::Content Lights[MAX_LIGHT_NUM];
	} content;

private:
	UINT mID;
	static UINT sIDCount;
};
