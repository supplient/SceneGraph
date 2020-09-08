#include "FbxLoader.h"
#include <locale>
#include <codecvt>

using namespace DirectX;

void XM_CALLCONV AxisTrans(FbxVector4 src, XMFLOAT3* dest, CXMMATRIX mat) {
	XMVECTOR vec = { (float)src[0], (float)src[1], (float)src[2], 0.0f };
	vec = XMVector3Transform(vec, mat);
	XMStoreFloat3(dest, vec);
}

std::shared_ptr<Texture> FbxLoader::LoadTexture(FbxFileTexture * tex)
{
	// Check if loaded
	if (mTexMappings.find(tex) != mTexMappings.end())
		return mTexMappings[tex];

	// Create Texture
	std::string name = tex->GetName();
	auto nTex = std::make_shared<Texture>(name);
	mTexMappings[tex] = nTex;

	// Load File Path
	// TODO need check whether abs path exists 
	//		& whether using rel path
	//		& whether using other formats
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	nTex->SetFilePath(conv.from_bytes(tex->GetFileName()));

	return nTex;
}

std::shared_ptr<Material> FbxLoader::LoadMaterial(FbxNode * node, FbxSurfaceMaterial * mtl)
{
	// Check if loaded
	if (mMtlMappings.find(mtl) != mMtlMappings.end())
		return mMtlMappings[mtl];

	// Helper Functions
	auto LoadDouble3 = [mtl](const char* propertyName, XMFLOAT4* dest) {
		FbxProperty prop = mtl->FindProperty(propertyName);
		if (prop.IsValid()) {
			FbxDouble3 tmp = prop.Get<FbxDouble3>();
			*dest = { (float)tmp[0], (float)tmp[1], (float)tmp[2], 1.0f };
		}
	};
	auto LoadDouble = [mtl](const char* propertyName, FLOAT* dest) {
		FbxProperty prop = mtl->FindProperty(propertyName);
		if (prop.IsValid()) {
			FbxDouble tmp = prop.Get<FbxDouble>();
			*dest = tmp;
		}
	};
	auto LoadTex = [mtl, this](const char* propertyName, UINT32* dest) {
		FbxProperty prop = mtl->FindProperty(propertyName);
		int texCount = prop.GetSrcObjectCount<FbxFileTexture>();
		if (texCount == 0)
			return;
		FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>(0);
		auto nTex = LoadTexture(tex);
		*dest = nTex->GetID();
	};

	// Property Names
	const char* sDiffuse = "DiffuseColor";
	const char* sSpecular = "SpecularColor";
	const char* sRoughness = "roughness";

	// Create Material
	std::string mtlName = mtl->GetName();
	std::shared_ptr<Material> nMtl = std::make_shared<Material>(mtlName);
	mMtlMappings[mtl] = nMtl;

	// Load Textures
	LoadTex(sDiffuse, &nMtl->mDiffuseTexID);

	// Load properties
	LoadDouble3(sDiffuse, &nMtl->mDiffuse);
	LoadDouble3(sSpecular, &nMtl->mSpecular);
	LoadDouble(sRoughness, &nMtl->mRoughness);

	// DEBUG set some not included properties
	nMtl->mLTCAmpTexID = Texture::FindByName("ggx_ltc_amp")->GetID();
	nMtl->mLTCMatTexID = Texture::FindByName("ggx_ltc_mat")->GetID();

	return nMtl;
}

std::shared_ptr<Mesh> XM_CALLCONV FbxLoader::LoadMesh(
	FbxNode* node, FbxMesh* mesh, CXMMATRIX axisTransMat
) {
	// Check if loaded
	if (mMeshMappings.find(mesh) != mMeshMappings.end()) {
		return mMeshMappings[mesh];
	}

	// Create Mesh
	std::string meshName = node->GetName();
	std::shared_ptr<Mesh> nMesh = std::make_shared<Mesh>(meshName);
	mMeshMappings[mesh] = nMesh;

	// Init Vertex Data
	std::vector<Vertex> verts;
	std::vector<UINT32> indices;

	// Load UVSetnames
	FbxStringList uvSetNames;
	mesh->GetUVSetNames(uvSetNames);
	if (uvSetNames.GetCount() < 1)
		throw "Lack of UV.";

	// Load Control Points
	FbxVector4* ctlPoints = mesh->GetControlPoints();

	// Check Material's mapping mode
	// Note: we only use first material layer
	//		We will never use multi-layer material
	auto mtlEle = mesh->GetElementMaterial(0);
	bool usingMtl = true;
	if (!mtlEle) {
		usingMtl = false;
	}
	bool mtlByPolygon = false;
	if(usingMtl)
		mtlByPolygon = mtlEle->GetMappingMode() == FbxLayerElement::eByPolygon;

	// Prepare for spliting SubMesh
	SubMesh nowSubMesh;
	nowSubMesh.startIndexLoc = 0;
	nowSubMesh.baseVertexLoc = 0;
	nowSubMesh.indexCount = 0;
	nowSubMesh.materialID = -1; // Note: ID here is Node's local ID

	// Just save the submesh if mapping mode is byallsame, or not using material
	if (!mtlByPolygon || !usingMtl) {
		nowSubMesh.indexCount = mesh->GetPolygonVertexCount();
		if (usingMtl)
			nowSubMesh.materialID = mtlEle->GetIndexArray().GetAt(0);
		else
			nowSubMesh.materialID = -1;
		nMesh->AddSubMesh(nowSubMesh);
	}

	// Load each Polygon
	int indexCount = 0;
	for (int pi = 0; pi < mesh->GetPolygonCount(); pi++) {
		int polygonSize = mesh->GetPolygonSize(pi);
		int windingDelta = polygonSize-1;
		for (int vi = 0; vi < polygonSize; vi++) {
			// Coordinate
			int ctlPointIndex = mesh->GetPolygonVertex(pi, vi);
			FbxVector4 coordinate = ctlPoints[ctlPointIndex];

			// Normal
			FbxVector4 normal;
			if (!mesh->GetPolygonVertexNormal(pi, vi, normal)) {
				if (!mesh->GenerateNormals())
					throw std::string("Generate normals failed.");
				mesh->GetPolygonVertexNormal(pi, vi, normal);
			}

			// UV
			FbxVector2 uv;
			// for (int uvSetIndex = 0; uvSetIndex < uvSetNames.GetCount(); uvSetIndex++) 
			{
				std::string uvSetName = uvSetNames.GetStringAt(0); // Now, we only access one UVSet.
				FbxGeometryElementUV* uvEle = mesh->GetElementUV(uvSetName.c_str());
				if (!uvEle)
					throw "Load UV failed.";
				
				bool refDirect = uvEle->GetReferenceMode()==FbxGeometryElement::eDirect;
				int uvi;

				switch (uvEle->GetMappingMode()) {
				case FbxGeometryElement::eByPolygonVertex:
					uvi = indexCount;
					break;
				case FbxGeometryElement::eByControlPoint:
					uvi = ctlPointIndex;
					break;
				default:
					throw "UV loading only support MappingMode of eByControlPoint and eByPolygonVertex";
				}
				if(!refDirect)
					uvi = uvEle->GetIndexArray().GetAt(uvi);
				uv = uvEle->GetDirectArray().GetAt(uvi);
			}
			
			// Tangent
			FbxVector4 tangent;
			{
				FbxGeometryElementTangent* ele = mesh->GetElementTangent(0); // Now, we only access one UVSet.
				if (!ele) {
					if (!mesh->GenerateTangentsDataForAllUVSets())
						throw "Generate Tangent Failed.";
					ele = mesh->GetElementTangent(0);
				}

				bool refDirect = ele->GetReferenceMode()==FbxGeometryElement::eDirect;
				int index;

				switch (ele->GetMappingMode()) {
				case FbxGeometryElement::eByPolygonVertex:
					index = indexCount;
					break;
				case FbxGeometryElement::eByControlPoint:
					index = ctlPointIndex;
					break;
				default:
					throw "Tangent loading only support MappingMode of eByControlPoint and eByPolygonVertex";
				}
				if(!refDirect)
					index = ele->GetIndexArray().GetAt(index);
				tangent = ele->GetDirectArray().GetAt(index);
			}

			// Make the vertex & change the axis system
			Vertex vert;
			AxisTrans(coordinate, &vert.pos, axisTransMat);
			AxisTrans(normal, &vert.normal, axisTransMat);
			AxisTrans(tangent, &vert.tangent, axisTransMat);
			vert.tex = { (float)uv[0], (float)uv[1] };

			// Fill into buffer
			verts.push_back(vert);
			if (!mRightHanded) // Handle the winding
				indices.push_back(indexCount);
			else {
				indices.push_back(indexCount + windingDelta);
				windingDelta -= 2;
			}
			// TODO indices now do not allow share the vertex

			indexCount++;
		} // PolygonSize

		// Check Material & Save SubMesh
		if (mtlByPolygon && usingMtl) {
			int matID = mtlEle->GetIndexArray().GetAt(pi);

			if (pi == mesh->GetPolygonCount() - 1 || 
				matID != mtlEle->GetIndexArray().GetAt(pi + 1)) 
			{
				// Last polygon or material will change, save nowSubMesh and start a new one
				nowSubMesh.materialID = matID;
				nowSubMesh.indexCount = indexCount;
				nMesh->AddSubMesh(nowSubMesh);
				
				nowSubMesh.startIndexLoc = indices.size();
				nowSubMesh.baseVertexLoc = verts.size();
				indexCount = 0;
			}
		}
	} // PolygonCount

	// Pass verts and indices into mesh
	nMesh->SetBuffer(verts, indices, DXGI_FORMAT_R32_UINT);

	return nMesh;
}


std::shared_ptr<Object> XM_CALLCONV FbxLoader::LoadObjectRecursively(
	FbxNode* rootNode, CXMMATRIX axisTransMat
) {
	if (!rootNode)
		return nullptr;
	// Node & Create Node
	std::string name(rootNode->GetName());
	auto rootObj = std::make_shared<Object>(name);

	// Transformation
	FbxDouble3 scaling = rootNode->LclScaling.Get();
	FbxDouble3 rotation = rootNode->LclRotation.Get();
	FbxDouble3 translation = rootNode->LclTranslation.Get();
	rootObj->SetScale((float)scaling[0], (float)scaling[1], (float)scaling[2]);
	rootObj->SetRotation((float)rotation[0], (float)rotation[1], (float)rotation[2]);
	rootObj->SetTranslation((float)translation[0], (float)translation[1], (float)translation[2]);

	// Materials
	std::vector<std::shared_ptr<Material>> mtlList;
	for (int mi = 0; mi < rootNode->GetMaterialCount(); mi++) {
		FbxSurfaceMaterial* mtl = rootNode->GetMaterial(mi);

		// Load material
		std::shared_ptr<Material> nMtl = LoadMaterial(rootNode, mtl);
		mtlList.push_back(nMtl);
	}
	
	// Attributes
	for (int ai = 0; ai < rootNode->GetNodeAttributeCount(); ai++) {
		FbxNodeAttribute* attr = rootNode->GetNodeAttributeByIndex(ai);

		if (attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
			FbxMesh* mesh = (FbxMesh*)(attr);
			if (!mesh)
				throw "The attribute's type is eMesh, while it cannot be converted to FbxMesh.";

			// Load mesh
			std::shared_ptr<Mesh> nMesh = LoadMesh(
				rootNode, mesh,
				axisTransMat
			);

			// Create renderItem
			for (UINT submeshID = 0; submeshID < nMesh->GetSubMeshNum(); submeshID++) {
				SubMesh submesh = nMesh->GetSubMesh(submeshID);

				UINT locMtlID = submesh.materialID;
				UINT glbMtlID;
				if (locMtlID == -1)
					glbMtlID = Material::GetDefaultMaterialID();
				else
					glbMtlID = mtlList[locMtlID]->GetID();

				auto renderItem = std::make_shared<RenderItem>();
				renderItem->MeshID = nMesh->GetID();
				renderItem->SubMeshID = submeshID;
				renderItem->MaterialID = glbMtlID;
				renderItem->PSO = "opaque";
				Object::Link(rootObj, renderItem);
			}
		}
		else {
			// Do nothing now.
			break;
		}
	}

	// Childs
	for (int i = 0; i < rootNode->GetChildCount(); i++) {
		auto childObj = LoadObjectRecursively(
			rootNode->GetChild(i),
			axisTransMat
			);
		Object::Link(rootObj, childObj);
	}
	return rootObj;
}


std::shared_ptr<Object> FbxLoader::Load(const char * filename)
{
	// Rest mappings
	mTexMappings.clear();
	mMtlMappings.clear();
	mMeshMappings.clear();
	
	// Make Root
	auto rootObject = std::make_shared<Object>("_root");

	// Initialize the SDK manager. This object handles all our memory management.
	FbxManager* lSdkManager = FbxManager::Create();

	// Create the IO settings object.
	FbxIOSettings *ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	// Create an importer using the SDK manager.
	FbxImporter* lImporter = FbxImporter::Create(lSdkManager,"");

	// Use the first argument as the filename for the importer.
	if(!lImporter->Initialize(filename, -1, lSdkManager->GetIOSettings())) {
		OutputDebugStringA("Call to FbxImporter::Initialize() failed.\n");
		std::string errorStr = "Error returned: ";
		errorStr += lImporter->GetStatus().GetErrorString();
		errorStr += "\n\n";
		OutputDebugStringA(errorStr.c_str());
		exit(-1);
	}

	// Create a new scene so that it can be populated by the imported file.
	FbxScene* lScene = FbxScene::Create(lSdkManager,"myScene");

	// Import the contents of the file into the scene.
	lImporter->Import(lScene);

	// The file is imported; so get rid of the importer.
	lImporter->Destroy();

	// Cal axis system transform matrix
	XMMATRIX axisTransMat = XMMatrixIdentity();
	{
		FbxGlobalSettings* globalSettings = &lScene->GetGlobalSettings();
		int upAxis = globalSettings->GetOriginalUpAxis();
		if (upAxis == 0) // X
			axisTransMat = XMMatrixRotationZ(MathHelper::Pi / 2.0f);
		else if (upAxis == 2) // Z
			axisTransMat = XMMatrixRotationX(-MathHelper::Pi / 2.0f);
		
		auto axisSystem = globalSettings->GetAxisSystem();
		if (axisSystem.GetCoorSystem() == FbxAxisSystem::eRightHanded) {
			axisTransMat *= XMMatrixScaling(1.0f, 1.0f, -1.0f);
			mRightHanded = true;
		}
		else
			mRightHanded = false;
	}

	// Recursively process the nodes of the scene and their attributes.
	FbxNode* lRootNode = lScene->GetRootNode();
	if(lRootNode) {
		for (int i = 0; i < lRootNode->GetChildCount(); i++) {
			auto childObj = LoadObjectRecursively(
				lRootNode->GetChild(i), 
				axisTransMat
			);
			Object::Link(rootObject, childObj);
		}
	}

	// Destroy the SDK manager and all the other objects it was handling.
	lSdkManager->Destroy();

	return rootObject;
}

std::vector<std::shared_ptr<Mesh>> FbxLoader::GetMeshs()
{
	std::vector<std::shared_ptr<Mesh>> res;
	for (auto& pair : mMeshMappings) {
		res.push_back(pair.second);
	}
	return res;
}

std::vector<std::shared_ptr<Material>> FbxLoader::GetMaterials()
{
	std::vector<std::shared_ptr<Material>> res;
	for (auto& pair : mMtlMappings)
		res.push_back(pair.second);
	return res;
}

std::vector<std::shared_ptr<Texture>> FbxLoader::GetTextures()
{
	std::vector<std::shared_ptr<Texture>> res;
	for (auto& pair : mTexMappings)
		res.push_back(pair.second);
	return res;
}
