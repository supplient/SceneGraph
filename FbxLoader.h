#pragma once
#include <fbxsdk.h>
#include "Common/d3dUtil.h"

#include "Material.h"
#include "Mesh.h"
#include "Object.h"
#include "RenderItem.h"	
#include "Texture.h"

// Type Define
// TODO consider about where to put this struct.
struct Vertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT2 tex;
};

class FbxLoader
{
public:
	std::shared_ptr<Object> Load(const char* filename);
	std::vector<std::shared_ptr<Mesh>> GetMeshs();
	std::vector<std::shared_ptr<Material>> GetMaterials();
	std::vector<std::shared_ptr<Texture>> GetTextures();

private:
	std::shared_ptr<Texture> LoadTexture(
		FbxFileTexture* tex);
	std::shared_ptr<Material> LoadMaterial(
		FbxNode* node, FbxSurfaceMaterial* mtl);
	std::shared_ptr<Mesh> XM_CALLCONV LoadMesh(
		FbxNode* node, FbxMesh* mesh, DirectX::CXMMATRIX axisTransMat);
	std::shared_ptr<Object> XM_CALLCONV LoadObjectRecursively(
		FbxNode* rootNode, DirectX::CXMMATRIX axisTransMat);

	std::unordered_map<FbxFileTexture*, std::shared_ptr<Texture>> mTexMappings;
	std::unordered_map<FbxMesh*, std::shared_ptr<Mesh>> mMeshMappings;
	std::unordered_map<FbxSurfaceMaterial*, std::shared_ptr<Material>> mMtlMappings;
};
