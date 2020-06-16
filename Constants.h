#pragma once
#include "../Common/d3dUtil.h"

class ObjectConstants {
public:
	ObjectConstants() {
		mID = sIDCount;
		sIDCount++;
	}
	UINT getID()const { return mID; }
	static UINT getTotalNum() { return sIDCount; }

	struct Content{
		DirectX::XMFLOAT4X4 MVPMat = MathHelper::Identity4x4();
	} content;

private:
	UINT mID;
	static UINT sIDCount;
};

