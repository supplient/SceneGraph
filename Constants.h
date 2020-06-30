#pragma once
#include "../Common/d3dUtil.h"
#include "Predefine.h"

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

class FxaaConstants {
public:
	FxaaConstants() {
		mID = sIDCount;
		sIDCount++;
	}
	UINT getID()const { return mID; }
	static UINT getTotalNum() { return sIDCount; }

	struct Content {
		FLOAT QualitySubpix;//1: pack count
		FLOAT QualityEdgeThreshold;//2
		DirectX::XMFLOAT2 QualityRcpFrame;//4
		DirectX::XMFLOAT4 ConsoleRcpFrameOpt;//4
		DirectX::XMFLOAT4 ConsoleRcpFrameOpt2;//4
		DirectX::XMFLOAT4 Console360RcpFrameOpt2;//4
		FLOAT QualityEdgeThresholdMin;//1
		FLOAT ConsoleEdgeSharpness;//2
		FLOAT ConsoleEdgeThreshold;//3
		FLOAT ConsoleEdgeThresholdMin;//4
		DirectX::XMFLOAT4 Console360ConstDir;//4
	} content;

private:
	UINT mID;
	static UINT sIDCount;
};
