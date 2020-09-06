#pragma once
#include "Common/d3dUtil.h"
#include "Predefine.h"

class SubMesh
{
public:
	UINT indexCount = 0;
	UINT startIndexLoc = 0;
	UINT baseVertexLoc = 0;
	D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT materialID = -1;
};

class Mesh
{
public:
	Mesh(const std::string& name)
		: mName(name)
	{
		mID = sIDCount;
		sIDCount++;
		sIDMap.push_back(this);
	}
	~Mesh() {
		sIDMap[mID] = nullptr;
	}

	std::string GetName()const { return mName; }
	UINT GetID()const { return mID; }
	static UINT GetTotalNum() { return sIDCount; }
	static Mesh* FindObjectByID(UINT id) {
		if (id >= sIDMap.size())
			return nullptr;
		return sIDMap[id];
	}
	
	template<class T, class U>
	void SetBuffer(std::vector<T> verts, std::vector<U> indices, DXGI_FORMAT indexFormat);
	void UploadBuffer(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void DisposeUploaders(); // TODO actually, we have never called this function.

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()const;
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()const;

	void AddSubMesh(const SubMesh& submesh) {
		mSubMeshs.push_back(submesh);
	}
	SubMesh GetSubMesh(UINT i) {
		if (i >= static_cast<UINT>(mSubMeshs.size()))
			throw "Out of bound";
		return mSubMeshs[i];
	}
	UINT GetSubMeshNum() { return mSubMeshs.size(); }

private:
	// Note: We left UINT32_MAX as an invalid ID.
	UINT mID;
	static UINT sIDCount;
	static std::vector<Mesh*> sIDMap;

	std::string mName;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUpload = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUpload = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> mVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mIndexBufferCPU = nullptr;

	UINT mVertexByteStride = 0;
	UINT mVertexBufferByteSize = 0;
	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R16_UINT;
	UINT mIndexBufferByteSize = 0;

	std::vector<SubMesh> mSubMeshs;
};


template<class T, class U>
void Mesh::SetBuffer(
	std::vector<T> verts, std::vector<U> indices, 
	DXGI_FORMAT indexFormat
) {
	// Fill Buffer Info
	mVertexByteStride = sizeof(T);
	mVertexBufferByteSize = static_cast<UINT>(verts.size() * sizeof(T));
	mIndexFormat = indexFormat;
	mIndexBufferByteSize = static_cast<UINT>(indices.size() * sizeof(U));

	// Save Buffer Data
	ThrowIfFailed(D3DCreateBlob(mVertexBufferByteSize, 
		mVertexBufferCPU.ReleaseAndGetAddressOf()));
	CopyMemory(mVertexBufferCPU->GetBufferPointer(), 
		verts.data(), 
		mVertexBufferByteSize);

	ThrowIfFailed(D3DCreateBlob(mIndexBufferByteSize, 
		mIndexBufferCPU.ReleaseAndGetAddressOf()));
	CopyMemory(mIndexBufferCPU->GetBufferPointer(), 
		indices.data(), 
		mIndexBufferByteSize);
}
