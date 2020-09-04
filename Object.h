#pragma once
#include "Common/d3dUtil.h"
#include "Predefine.h"

class Object
{
public:
	static bool Link(std::shared_ptr<Object> parent, std::shared_ptr<Object> child)
	{
		if (child->mParent)
			return false;
		parent->mChilds.push_back(child);
		child->mParent = parent;
	}

public:
	Object(const std::string& name)
		: mName(name)
	{}

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
	}

	void SetTranslation(float x, float y, float z) {
		mTranslation = { x, y, z };
	}
	void SetRotation(float x, float y, float z) {
		mRotation = { x, y, z };
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

private:
	std::string mName;

	std::shared_ptr<Object> mParent = nullptr;
	std::vector<std::shared_ptr<Object>> mChilds;

	DirectX::XMFLOAT3 mTranslation;
	DirectX::XMFLOAT3 mRotation;
	DirectX::XMFLOAT3 mScale;
	DirectX::XMFLOAT4X4 mGlobalModelMat;

};
