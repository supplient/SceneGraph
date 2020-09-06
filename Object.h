#pragma once
#include "Common/d3dUtil.h"
#include "Predefine.h"
#include "RenderItem.h"

class Object
{
public:
	static bool Link(std::shared_ptr<Object> parent, std::shared_ptr<Object> child)
	{
		if (child->mParent)
			return false;
		parent->mChilds.push_back(child);
		child->mParent = parent;
		return true;
	}

	static bool Link(std::shared_ptr<Object> obj, std::shared_ptr<RenderItem> item)
	{
		if (item->ObjectID != INVALID_OBJECT_ID)
			return false;
		obj->mRenderItems.push_back(item);
		item->ObjectID = obj->mID;
		return true;
	}

public:
	Object(const std::string& name)
		: mName(name)
	{
		mID = sIDCount; 
		sIDCount++;
		sIDMap.push_back(this);
	}
	~Object() {
		sIDMap[mID] = nullptr;
	}

	UINT GetID()const { return mID; }
	static UINT GetTotalNum() { return sIDCount; }
	static Object* FindObjectByID(UINT id) {
		if (id >= sIDMap.size())
			return nullptr;
		return sIDMap[id];
	}

	struct Content {
		DirectX::XMFLOAT4X4 ModelMat = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 NormalModelMat = MathHelper::Identity4x4();
	};
	Content ToContent()const {
		Content content;

		// cal ModelMat
		DirectX::XMMATRIX mat = GetGlobalModelMat();

		// cal NormalModelMat
		DirectX::XMMATRIX normalMat = MathHelper::GenNormalMat(mat);

		// HLSL use column-major
		DirectX::XMStoreFloat4x4(
			&content.ModelMat,
			DirectX::XMMatrixTranspose(mat)
		);
		DirectX::XMStoreFloat4x4(
			&content.NormalModelMat,
			DirectX::XMMatrixTranspose(normalMat)
		);

		return content;
	}

	void SetTranslation(float x, float y, float z) {
		mTranslation = { x, y, z };
	}
	void SetRotation(float x, float y, float z) {
		mRotation = { x, y, z };
	}
	void SetRotation_Degree(float x, float y, float z) {
		mRotation = { MathHelper::AngleToRadius(x), MathHelper::AngleToRadius(y), MathHelper::AngleToRadius(z) };
	}
	void SetScale(float x, float y, float z) {
		mScale = { x, y, z };
	}

	DirectX::XMMATRIX CalLocalModelMat()const {
		DirectX::XMMATRIX mat = DirectX::XMMatrixScaling(mScale.x, mScale.y, mScale.z);
		mat *= DirectX::XMMatrixRotationX(mRotation.x);
		mat *= DirectX::XMMatrixRotationY(mRotation.y);
		mat *= DirectX::XMMatrixRotationZ(mRotation.z);
		mat *= DirectX::XMMatrixTranslation(
			mTranslation.x,
			mTranslation.y,
			mTranslation.z);
		return mat;
	}

	void UpdateGlobalModelMatRecursively() {
		auto globalMat = CalLocalModelMat();
		DirectX::XMStoreFloat4x4(&mGlobalModelMat, globalMat);

		for (auto& child : mChilds)
			child->UpdateGlobalModelMatRecursively(globalMat);
	}
	void XM_CALLCONV UpdateGlobalModelMatRecursively(DirectX::CXMMATRIX parentMat) {
		auto localMat = CalLocalModelMat();
		auto globalMat = localMat * parentMat;
		DirectX::XMStoreFloat4x4(&mGlobalModelMat, globalMat);

		for (auto& child : mChilds)
			child->UpdateGlobalModelMatRecursively(globalMat);
	}
	DirectX::XMMATRIX GetGlobalModelMat()const {
		return DirectX::XMLoadFloat4x4(&mGlobalModelMat);
	}

	std::vector<std::shared_ptr<Object>> GetChilds() { return mChilds; }
	std::vector<std::shared_ptr<RenderItem>> GetRenderItems() { return mRenderItems; }

private:
	// Note: We left UINT32_MAX as an invalid ID.
	UINT mID;
	static UINT sIDCount;
	static std::vector<Object*> sIDMap;

	std::string mName;

	std::shared_ptr<Object> mParent = nullptr;
	std::vector<std::shared_ptr<Object>> mChilds;
	std::vector<std::shared_ptr<RenderItem>> mRenderItems;

	DirectX::XMFLOAT3 mTranslation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRotation = { 0.0f, 0.0f, 0.0f };;
	DirectX::XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };;
	DirectX::XMFLOAT4X4 mGlobalModelMat;
};
