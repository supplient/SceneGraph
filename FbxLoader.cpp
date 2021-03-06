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
	auto LoadDouble4 = [mtl](FbxProperty prop, XMFLOAT4* dest) {
		FbxColor tmp = prop.Get<FbxColor>();
		*dest = { (float)tmp.mRed, (float)tmp.mGreen, (float)tmp.mBlue, (float)tmp.mAlpha };
	};
	auto LoadDouble = [mtl](FbxProperty prop, FLOAT* dest) {
		FbxDouble tmp = prop.Get<FbxDouble>();
		*dest = (float)tmp;
	};
	auto LoadBool = [mtl](FbxProperty prop, bool* dest) {
		FbxBool tmp = prop.Get<FbxBool>();
		*dest = (float)tmp;
	};
	auto LoadTex = [mtl, this](FbxProperty prop, UINT32* dest) {
		int texCount = prop.GetSrcObjectCount<FbxFileTexture>();
		if (texCount == 0)
			return;
		FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>(0);
		auto nTex = LoadTexture(tex);
		*dest = nTex->GetID();
	};

	// Property Names
	const char* sBaseColor = "base_color";
	const char* sMetalness = "metalness";
	const char* sIOR = "trans_ior";
	const char* sRoughness = "roughness";
	const char* sRoughnessInv = "roughness_inv";
	const char* sBaseColorTex = "DiffuseColor";

	// Create Material
	std::string mtlName = mtl->GetName();
	std::shared_ptr<Material> nMtl = std::make_shared<Material>(mtlName);
	mMtlMappings[mtl] = nMtl;

	bool roughnessInv = false;
	FbxProperty prop = mtl->GetFirstProperty();
	while (prop.IsValid()) {
		FbxString propName = prop.GetName();
		auto is = [&propName](const char *name) { return propName == name; };

		if (is(sBaseColor))
			LoadDouble4(prop, &nMtl->mBaseColor);
		else if (is(sMetalness))
			LoadDouble(prop, &nMtl->mMetalness);
		else if (is(sIOR))
			LoadDouble(prop, &nMtl->mIOR);
		else if (is(sRoughness))
			LoadDouble(prop, &nMtl->mRoughness);
		else if (is(sRoughnessInv))
			LoadBool(prop, &roughnessInv);
		else if (is(sBaseColorTex))
			LoadTex(prop, &nMtl->mBaseColorTexID);

		prop = mtl->GetNextProperty(prop);
	}

	if (roughnessInv)
		nMtl->mRoughness = 1.0f - nMtl->mRoughness;

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
				
				nowSubMesh.startIndexLoc = static_cast<UINT>(indices.size());
				nowSubMesh.baseVertexLoc = static_cast<UINT>(verts.size());
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
	// Handle axis trans && Set
	{
		// Scale
		{
			double x, y, z;
			// Determine each axis's responding
			if (mUpAxis == 0) {
				// X-up
				x = scaling[1];
				y = scaling[0];
				z = scaling[2];
			}
			else if (mUpAxis == 1) {
				// Y-up
				x = scaling[0];
				y = scaling[1];
				z = scaling[2];
			}
			else if (mUpAxis == 2) {
				// Z-up
				x = scaling[0];
				y = scaling[2];
				z = scaling[1];
			}
			else
				throw "Error";
			rootObj->SetScale((float)x, (float)y, (float)z);
		}
		// Rotation
		{
			double mx = rotation[0], my = rotation[1], mz = rotation[2];
			double x, y, z;
			if (!mRightHanded && mUpAxis == 0) {
				x = -my;
				y = mx;
				z = mz;
			}
			else if (!mRightHanded && mUpAxis == 1) {
				x = mx;
				y = my;
				z = mz;
			}
			else if (!mRightHanded && mUpAxis == 2) {
				x = mx;
				y = mz;
				z = -my;
			}
			else if (mRightHanded && mUpAxis == 0) {
				x = my;
				y = -mx;
				z = mz;
			}
			else if (mRightHanded && mUpAxis == 1) {
				x = -mx;
				y = -my;
				z = mz;
			}
			else if (mRightHanded && mUpAxis == 2) {
				x = -mx;
				y = -mz;
				z = -my;
			}
			else
				throw "Error";
			rootObj->SetRotation_Degree((float)x, (float)y, (float)z);
		}
		// Translation
		{
			double mx = translation[0], my = translation[1], mz = translation[2];
			double x, y, z;
			if (mUpAxis == 0) {
				x = -my;
				y = mx;
				z = mz;
			}
			else if (mUpAxis == 1) {
				x = mx;
				y = my;
				z = mz;
			}
			else if (mUpAxis == 2) {
				x = mx;
				y = mz;
				z = -my;
			}
			else
				throw "Error";

			if (mRightHanded)
				z = -z;
			rootObj->SetTranslation((float)x, (float)y, (float)z);
		}
	}

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
		mUpAxis = upAxis;
		
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
