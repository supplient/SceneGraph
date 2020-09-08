#pragma once

#include "Common/d3dUtil.h"
#include "Predefine.h"

class Material
{
public:
	Material(const std::string& name)
		: mName(name)
	{
		mID = sIDCount;
		sIDCount++;
		sIDMap.push_back(this);
	}
	~Material() {
		sIDMap[mID] = nullptr;
	}
	
	UINT GetID()const { return mID; }
	static UINT GetTotalNum() { return sIDCount; }
	static Material* FindObjectByID(UINT id) {
		if (id >= sIDMap.size())
			return nullptr;
		return sIDMap[id];
	}

	static UINT GetDefaultMaterialID() { return sDefaultMtlID; }
	static void SetDefaultMaterialID(UINT id) { sDefaultMtlID = id; }

	std::string GetName() { return mName; }

	struct Content {
		DirectX::XMFLOAT4 Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f }; // subsurface albedo
		DirectX::XMFLOAT4 Specular = { 0.04f, 0.04f, 0.04f, 0.04f }; // normal incidence Fersnel reflectance, F0
		FLOAT Roughness = 0.5f; 
		UINT32 LTCMatTexID = -1; // TexID -1 means not using this texture
		UINT32 LTCAmpTexID = -1;
		UINT32 DiffuseTexID = -1;
		UINT32 DispTexID = -1;
		FLOAT DispHeightScale = 1.0f; // grey value in DispTex(0.0f, 1.0f) * DispHeightScale = height in world space
		UINT32 NormalTexID = -1;
		FLOAT AlphaTestTheta = 1.0f; // if 1.0, means do not use alpha test
	} content;
	Content ToContent()const {
		Content content;

		content.Diffuse = mDiffuse;
		content.Specular = mSpecular;
		content.Roughness = mRoughness;

		content.LTCMatTexID = mLTCMatTexID;
		content.LTCAmpTexID = mLTCAmpTexID;

		content.DiffuseTexID = mDiffuseTexID;

		return content;
	}

	DirectX::XMFLOAT4 mDiffuse = { 1.0f, 1.0f, 1.0f, 1.0f }; // subsurface albedo;
	DirectX::XMFLOAT4 mSpecular = { 0.04f, 0.04f, 0.04f, 0.04f }; // normal incidence Fersnel reflectance, F0
	FLOAT mRoughness = 0.5f;
	UINT32 mLTCMatTexID = -1;
	UINT32 mLTCAmpTexID = -1;
	UINT32 mDiffuseTexID = -1;

private:
	UINT mID;
	static UINT sIDCount;
	static std::vector<Material*> sIDMap;
	static UINT sDefaultMtlID;

	std::string mName;
};
