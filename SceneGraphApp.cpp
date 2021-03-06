//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "SceneGraphApp.h"
#include "Common/GeometryGenerator.h"
#include "Predefine.h"
#include "PIXHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

template <class T, class U>
void FillBufferInfoAndUpload(
		ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList,
		std::shared_ptr<MeshGeometry> geo, 
		std::vector<T> verts, std::vector<U> indices, 
		DXGI_FORMAT indexFormat) {
	// Fill buffer info
	UINT vertByteSize = static_cast<UINT>(verts.size() * sizeof(T));
	UINT indexByteSize = static_cast<UINT>(indices.size() * sizeof(U));
	geo->VertexByteStride = sizeof(T);
	geo->VertexBufferByteSize = vertByteSize;
	geo->IndexFormat = indexFormat;
	geo->IndexBufferByteSize = indexByteSize;

	// Upload to GPU
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		device.Get(), commandList.Get(),
		(const void*)verts.data(), vertByteSize,
		geo->VertexBufferUploader
	);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		device.Get(), commandList.Get(),
		(const void*)indices.data(), indexByteSize,
		geo->IndexBufferUploader
	);
}

SceneGraphApp::SceneGraphApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

SceneGraphApp::~SceneGraphApp()
{
}

bool SceneGraphApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	bool fromFile = true;

	// Init Scene
	BuildManualTextures();
	BuildManualMaterials();
	BuildManualMeshs();
	if(fromFile)
		LoadScene();
	else
		BuildObjects();
	BuildManualObjects();
	BuildRenderItemQueueRecursively(mRootObject);
	BuildLights();
	BuildLightShadowConstantBuffers();

	// Init DirectX
	BuildUABs();
	BuildRenderTargets();
	BuildDescriptorHeaps();

	// Init PSO concerned
	BuildInputLayout();
	BuildRootSignature();
	BuildShaders();
	BuildPSOs();
	
	// Init Scene Resources
	LoadTextures();
	BuildPassConstants();
	BuildPassConstantBuffers();
	UpdateLightsInPassConstantBuffers();
	// BuildGeos();
	BuildAndUpdateMaterialConstantBuffers();

	// Init Render Item Resources
	BuildObjectConstantBuffers();

	// Init Postprocess Resources
	InitHbao();
	InitFxaa();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

    // Do the initial resize code.
    OnResize();

	return true;
}

void SceneGraphApp::BuildManualTextures()
{
	std::vector<std::shared_ptr<Texture>> texs;
	auto CreateTex = [&texs](std::string name, std::wstring filename) {
		auto tex = std::make_shared<Texture>(name);
		tex->SetFilePath(TEXTURE_PATH_HEAD + filename);
		texs.push_back(tex);
	};
	CreateTex("color", L"w_color.dds");
	CreateTex("height", L"w_height.dds");
	CreateTex("normal", L"w_normal.dds");
	CreateTex("tree", L"tree.dds");
	CreateTex("ggx_ltc_mat", L"ggx_ltc_mat.dds");
	CreateTex("ggx_ltc_amp", L"ggx_ltc_amp.dds");

	mTextures.insert(mTextures.end(), texs.begin(), texs.end());
}

void SceneGraphApp::BuildManualMeshs()
{
	// Background
	{
		struct Vertex_in {
			XMFLOAT3 pos;
		};

		// Create Mesh
		auto mesh = std::make_shared<Mesh>("background");

		// Save mesh
		mMeshs.push_back(mesh);

		// Generate Vertex Data
		std::vector<Vertex_in> verts = {
			{{-1.0f, -1.0f, 0.0f}},
			{{+1.0f, -1.0f, 0.0f}},
			{{+1.0f, +1.0f, 0.0f}},
			{{-1.0f, +1.0f, 0.0f}},
		};
		std::vector<UINT32> indices = {
			0, 2, 1,
			0, 3, 2,
		};

		// Create submesh
		SubMesh submesh;
		submesh.baseVertexLoc = 0;
		submesh.startIndexLoc = 0;
		submesh.indexCount = static_cast<UINT>(indices.size());
		mesh->AddSubMesh(submesh);

		// Pass Vertex Data into Mesh
		mesh->SetBuffer(verts, indices, DXGI_FORMAT_R32_UINT);

		// Upload mesh
		mesh->UploadBuffer(md3dDevice, mCommandList);
	}
}

void SceneGraphApp::LoadScene()
{
	FbxLoader loader;
	mRootObject = loader.Load("bear.fbx");
	auto meshs = loader.GetMeshs();
	auto mtls = loader.GetMaterials();
	auto texs = loader.GetTextures();

	// Save & Upload meshs
	for (auto mesh : meshs) {
		mMeshs.push_back(mesh);
		mesh->UploadBuffer(md3dDevice, mCommandList);
	}

	// Save materials
	for (auto mtl : mtls) {
		mMaterials.push_back(mtl);
	}

	// Save Textures
	for (auto tex : texs) {
		mTextures.push_back(tex);
	}
}

void SceneGraphApp::BuildObjects()
{
	// Make Root
	mRootObject = std::make_shared<Object>("_root");

	// Cube
	/*
	{
		// mid
		{
			// Create Object
			auto obj = std::make_shared<Object>("midCube");
			obj->SetScale(0.1f, 0.1f, 0.1f);
			Object::Link(mRootObject, obj);

			// Create render item
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->MeshPool = "triangle";
			renderItem->SubMesh = "triangle";
			renderItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->Material = "white";
			renderItem->PSO = "opaque";
			renderItem->ObjectID = obj->GetID();

			// Save render item
			mOpaqueRenderItemQueue.push_back(std::move(renderItem));
		}

		// bottom
		{
			// Create Object
			auto obj = std::make_shared<Object>("bottomCube");
			obj->SetScale(2.0f, 0.1f, 2.0f);
			obj->SetTranslation(0.0f, -1.05f, 0.0f);
			Object::Link(mRootObject, obj);

			// Create render item
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->MeshPool = "triangle";
			renderItem->SubMesh = "triangle";
			renderItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->Material = "white";
			renderItem->PSO = "opaque";
			renderItem->ObjectID = obj->GetID();

			// Save render item
			mOpaqueRenderItemQueue.push_back(std::move(renderItem));
		}

		// left
		{
			// Create Object
			auto obj = std::make_shared<Object>("leftCube");
			obj->SetScale(0.1f, 2.0f, 2.0f);
			obj->SetTranslation(-1.05f, 0.0f, 0.0f);
			Object::Link(mRootObject, obj);

			// Create render item
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->MeshPool = "triangle";
			renderItem->SubMesh = "triangle";
			renderItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->Material = "white";
			renderItem->PSO = "opaque";
			renderItem->ObjectID = obj->GetID();

			// Save render item
			mOpaqueRenderItemQueue.push_back(std::move(renderItem));
		}

		// back
		{
			// Create Object
			auto obj = std::make_shared<Object>("backCube");
			obj->SetScale(2.0f, 2.0f, 0.1f);
			obj->SetTranslation(0.0f, 0.0f, 1.05f);
			Object::Link(mRootObject, obj);

			// Create render item
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->MeshPool = "triangle";
			renderItem->SubMesh = "triangle";
			renderItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->Material = "white";
			renderItem->PSO = "opaque";
			renderItem->ObjectID = obj->GetID();

			// Save render item
			mOpaqueRenderItemQueue.push_back(std::move(renderItem));
		}
	}
	*/

}

void SceneGraphApp::BuildManualObjects()
{
	// Post
	{
		auto obj = std::make_shared<Object>("background");
		Object::Link(mRootObject, obj);
		
		auto renderItem = std::make_shared<RenderItem>();
		renderItem->MeshID = Mesh::FindMeshByName("background")->GetID();
		renderItem->SubMeshID = 0;
		renderItem->MaterialID = Material::GetDefaultMaterialID();
		renderItem->ObjectID = obj->GetID();

		// Save render items
		mBackgroundRenderItem = std::move(renderItem);
	}
}

void SceneGraphApp::BuildRenderItemQueueRecursively(std::shared_ptr<Object> root)
{
	for (auto renderItem : root->GetRenderItems()) {
		Mesh* mesh = Mesh::FindObjectByID(renderItem->MeshID);
		if (mesh->GetName() == "background")
			continue;
		// TODO now we just push all renderItems into opaqueQueue
		mOpaqueRenderItemQueue.push_back(renderItem);
	}
	for (auto child : root->GetChilds())
		BuildRenderItemQueueRecursively(child);
}

void SceneGraphApp::BuildLights()
{
	mDirLights.push_back({
		// {1.6f, 1.6f, 1.6f},
		{0.4f, 0.4f, 0.4f},
		{0.0f, 0.0f, 1.0f}
	});

	mPointLights.push_back({
		// {4.0f, 0.4f, 0.2f},
		{0.4f, 0.4f, 0.4f},
		{0.0f, 2.0f, 0.0f},
		0.01f, 10.0f
	});

	mSpotLights.push_back({
		// {0.2f, 4.0f, 0.4f},
		{0.4f, 0.4f, 0.4f},
		{2.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		0.01f, 10.0f,
		10.0f, 45.0f
	});

	/*
	mRectLights.push_back({
		{0.2f, 0.2f, 1.0f},
		{-0.2f, 0.5f, 0.5f}, 
		{-0.2f, 0.5f, -0.5f},
		{-0.2f, -0.5f, -0.5f}
	});
	*/

	// Cal shadow pass constants
	for (auto& dirLight : mDirLights) {
		dirLight.PassConstants = std::make_unique<ShadowPassConstants>();
	}
	for (auto& pointLight : mPointLights) {
		auto viewMats = pointLight.CalLightViewMats();
		auto projMat = pointLight.CalLightProjMat();

		for (UINT i = 0; i < 6; i++) {
			pointLight.PassConstantsArray[i] = std::make_unique<ShadowPassConstants>();
			auto& content = pointLight.PassConstantsArray[i]->content;

			XMStoreFloat4x4(
				&content.ViewMat, 
				XMMatrixTranspose(viewMats[i])
			);
			XMStoreFloat4x4(
				&content.ProjMat, 
				XMMatrixTranspose(projMat)
			);
		}
	}
	for (auto& spotLight : mSpotLights) {
		spotLight.PassConstants = std::make_unique<ShadowPassConstants>();
		auto& content = spotLight.PassConstants->content;

		XMStoreFloat4x4(
			&content.ViewMat, 
			XMMatrixTranspose(spotLight.CalLightViewMat())
		);
		XMStoreFloat4x4(
			&content.ProjMat, 
			XMMatrixTranspose(spotLight.CalLightProjMat())
		);
	}
}

void SceneGraphApp::BuildLightShadowConstantBuffers()
{
	mShadowPassConstantsBuffers = std::make_unique<UploadBuffer<ShadowPassConstants::Content>>(
		md3dDevice.Get(), 
		max(ShadowPassConstants::getTotalNum(), 1), 
		true
	);
}

void SceneGraphApp::BuildUABs() 
{
	UINT zeroUINT[4];
	zeroUINT[0] = 0;
	zeroUINT[1] = 0;
	zeroUINT[2] = 0;
	zeroUINT[3] = 0;
	FLOAT zeroFLOAT[4];
	zeroFLOAT[0] = 0.0f;
	zeroFLOAT[1] = 0.0f;
	zeroFLOAT[2] = 0.0f;
	zeroFLOAT[3] = 0.0f;

	mUABs["nCount"] = std::make_unique<UnorderedAccessBuffer>(
		L"nCount", DXGI_FORMAT_R32_UINT, (FLOAT*)zeroUINT
		);
}

void SceneGraphApp::BuildRenderTargets()
{
	FLOAT whiteClearValue[4];
	whiteClearValue[0] = 0.0f;
	whiteClearValue[1] = 0.0f;
	whiteClearValue[2] = 0.0f;
	whiteClearValue[3] = 0.0f;
	FLOAT maxClearValue[4];
	maxClearValue[0] = D3D12_FLOAT32_MAX;
	maxClearValue[1] = D3D12_FLOAT32_MAX;
	maxClearValue[2] = D3D12_FLOAT32_MAX;
	maxClearValue[3] = D3D12_FLOAT32_MAX;

	// Build Single Render Targets
	mRenderTargets["opaque"] = std::make_shared<SingleRenderTarget>(
		L"opaque", DXGI_FORMAT_R8G8B8A8_UNORM, whiteClearValue,
		true, L"opaque depth", DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT
		);
	/*
	mRenderTargets["hbao"] = std::make_shared<SingleRenderTarget>(
		L"hbao", DXGI_FORMAT_R8G8B8A8_UNORM, whiteClearValue
		);
	mRenderTargets["trans"] = std::make_shared<SingleRenderTarget>(
		L"transparent", DXGI_FORMAT_R32G32B32A32_FLOAT, whiteClearValue
		);
	mRenderTargets["transBlend"] = std::make_shared<SingleRenderTarget>(
		L"transparent blend", DXGI_FORMAT_R8G8B8A8_UNORM, whiteClearValue);
	*/
	mRenderTargets["afterResolve"] = std::make_shared<SingleRenderTarget>(
		L"afterResolve", DXGI_FORMAT_R8G8B8A8_UNORM, whiteClearValue);
	mRenderTargets["fxaa"] = std::make_shared<SingleRenderTarget>(
		L"fxaa", DXGI_FORMAT_R8G8B8A8_UNORM, whiteClearValue,
		true, L"fxaa depth"
		);

	// Build SwapChain
	DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1; // Note, DirectX12 does not support SwapChain MSAA
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	UINT rtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UINT srvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mSwapChain = std::make_unique<SwapChainRenderTarget>(
		L"swap chain",
		mdxgiFactory, mCommandQueue,
		&sd, whiteClearValue,
		rtvDescriptorSize, srvDescriptorSize
		);

	// Build Shadow Render Targets
	unsigned int index = 0;
	for (auto& dirLight : mDirLights) {
		dirLight.ShadowRT = std::make_unique<SingleRenderTarget>(
			L"Shadow Dir " + std::to_wstring(index),
			SHADOW_RTV_FORMAT, whiteClearValue,
			true,
			L"Shadow Dir Depth" + std::to_wstring(index),
			SHADOW_DSV_RESOURCE_FORMAT,
			SHADOW_DSV_VIEW_FORMAT,
			1.0f, 0
			);
	}

	index = 0;
	for (auto& pointLight : mPointLights) {
		pointLight.ShadowRT = std::make_unique<CubeRenderTarget>(
			L"Shadow Point " + std::to_wstring(index),
			SHADOW_RTV_FORMAT, maxClearValue,
			mCbvSrvUavDescriptorSize,
			true,
			L"Shadow Point Depth" + std::to_wstring(index),
			SHADOW_DSV_RESOURCE_FORMAT,
			SHADOW_DSV_VIEW_FORMAT,
			1.0f, 0
			);
	}

	index = 0;
	for (auto& spotLight : mSpotLights) {
		spotLight.ShadowRT = std::make_unique<SingleRenderTarget>(
			L"Shadow Spot " + std::to_wstring(index),
			SHADOW_RTV_FORMAT, whiteClearValue,
			true,
			L"Shadow Spot Depth" + std::to_wstring(index),
			SHADOW_DSV_RESOURCE_FORMAT,
			SHADOW_DSV_VIEW_FORMAT,
			1.0f, 0
			);
	}
}

void SceneGraphApp::BuildDescriptorHeaps()
{
	// DSV
	{
		// Check who need DepthStencilView among RenderTargets
		std::vector<std::string> renderTargetsNeedDS;
		for (auto& pair : mRenderTargets) {
			std::string name = pair.first;
			auto& renderTarget = pair.second;
			if (renderTarget->CheckHasDepthStencil())
				renderTargetsNeedDS.push_back(name);
		}

		// Build Descriptor Heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 
			(UINT)renderTargetsNeedDS.size()
			+ (UINT)mSpotLights.size() 
			+ (UINT)mDirLights.size()
			+ (UINT)mPointLights.size();
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		mDSVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		// Build Handle
		for (std::string name : renderTargetsNeedDS) {
			mRenderTargets[name]->SetDSVCPUHandle(mDSVHeap->GetCPUHandle(mDSVHeap->Alloc()));
		}

		for (auto& spotLight : mSpotLights)
			spotLight.ShadowRT->SetDSVCPUHandle(mDSVHeap->GetCPUHandle(mDSVHeap->Alloc()));
		for (auto& pointLight : mPointLights)
			pointLight.ShadowRT->SetDSVCPUHandle(mDSVHeap->GetCPUHandle(mDSVHeap->Alloc()));
		for (auto& dirLight : mDirLights)
			dirLight.ShadowRT->SetDSVCPUHandle(mDSVHeap->GetCPUHandle(mDSVHeap->Alloc()));
	}

	// RTV
	{
		// Build Descriptor Heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 
			(UINT)mRenderTargets.size() 
			+ mSwapChain->GetSwapChainBufferCount() 
			+ (UINT)mSpotLights.size()
			+ (UINT)mDirLights.size()
			+ (UINT)mPointLights.size() * PointLight::RTVNum;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		mRTVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		// Build Handle
		for (auto& pair : mRenderTargets) {
			auto& renderTarget = pair.second;
			renderTarget->SetRTVCPUHandle(mRTVHeap->GetCPUHandle(mRTVHeap->Alloc()));
		}
		UINT index = mRTVHeap->Alloc(mSwapChain->GetSwapChainBufferCount());
		mSwapChain->SetRTVCPUHandleStart(mRTVHeap->GetCPUHandle(index));

		for (auto& spotLight : mSpotLights)
			spotLight.ShadowRT->SetRTVCPUHandle(mRTVHeap->GetCPUHandle(mRTVHeap->Alloc()));
		for (auto& pointLight : mPointLights) {
			index = mRTVHeap->Alloc(PointLight::RTVNum);
			pointLight.ShadowRT->SetRTVCPUHandleStart(mRTVHeap->GetCPUHandle(index));
		}
		for (auto& dirLight : mDirLights)
			dirLight.ShadowRT->SetRTVCPUHandle(mRTVHeap->GetCPUHandle(mRTVHeap->Alloc()));
	}

	// CBV, SRV, UAV // GPU heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 
			1
			+ (UINT)mUABs.size()
			+ (UINT)mRenderTargets.size() 
			+ mSwapChain->GetSwapChainBufferCount() 
			+ (UINT)mTextures.size() 
			+ (UINT)mSpotLights.size()
			+ (UINT)mDirLights.size()
			+ (UINT)mPointLights.size();
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;
		mCBVSRVUAVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		UINT index;
		for (auto& pair : mRenderTargets) {
			auto& renderTarget = pair.second;
			index = mCBVSRVUAVHeap->Alloc();
			renderTarget->SetSRVCPUHandle(mCBVSRVUAVHeap->GetCPUHandle(index));
			renderTarget->SetSRVGPUHandle(mCBVSRVUAVHeap->GetGPUHandle(index));
		}

		index = mCBVSRVUAVHeap->Alloc(mSwapChain->GetSwapChainBufferCount());
		mSwapChain->SetSRVCPUHandleStart(mCBVSRVUAVHeap->GetCPUHandle(index));
		mSwapChain->SetSRVGPUHandleStart(mCBVSRVUAVHeap->GetGPUHandle(index));

		index = mCBVSRVUAVHeap->Alloc();
		mZBufferSRVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mZBufferSRVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);

		for (auto& pair : mUABs) {
			auto& uab = pair.second;
			index = mCBVSRVUAVHeap->Alloc();
			uab->SetCPUHandle(mCBVSRVUAVHeap->GetCPUHandle(index));
			uab->SetGPUHandle(mCBVSRVUAVHeap->GetGPUHandle(index));
		}

		index = mCBVSRVUAVHeap->Alloc((UINT)mTextures.size());
		mTexGPUHandleStart = mCBVSRVUAVHeap->GetGPUHandle(index);
		mTexCPUHandleStart = mCBVSRVUAVHeap->GetCPUHandle(index);

		index = mCBVSRVUAVHeap->Alloc((UINT)mSpotLights.size());
		mSpotShadowTexCPUHandleStart = mCBVSRVUAVHeap->GetCPUHandle(index);
		mSpotShadowTexGPUHandleStart = mCBVSRVUAVHeap->GetGPUHandle(index);

		index = mCBVSRVUAVHeap->Alloc((UINT)mDirLights.size());
		mDirShadowTexCPUHandleStart = mCBVSRVUAVHeap->GetCPUHandle(index);
		mDirShadowTexGPUHandleStart = mCBVSRVUAVHeap->GetGPUHandle(index);

		index = mCBVSRVUAVHeap->Alloc((UINT)mPointLights.size());
		mPointShadowTexCPUHandleStart = mCBVSRVUAVHeap->GetCPUHandle(index);
		mPointShadowTexGPUHandleStart = mCBVSRVUAVHeap->GetGPUHandle(index);
	}

	// CBV, SRV, UAV // CPU heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;

		mCBVSRVUAVCPUHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		UINT index;
		for (auto& pair : mUABs) {
			auto& uab = pair.second;
			index = mCBVSRVUAVCPUHeap->Alloc();
			uab->SetCPUHandle_CPUHeap(mCBVSRVUAVCPUHeap->GetCPUHandle(index));
		}
	}

	// Light Shadow RenderTarget's SRV Handle Set
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mSpotShadowTexCPUHandleStart);
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mSpotShadowTexGPUHandleStart);
		for (UINT i = 0; i < mSpotLights.size(); i++) {
			auto& spotLight = mSpotLights[i];
			spotLight.ShadowSRVID = i;
			spotLight.ShadowRT->SetSRVCPUHandle(cpuHandle);
			spotLight.ShadowRT->SetSRVGPUHandle(gpuHandle);
			cpuHandle.Offset(1, mCbvSrvUavDescriptorSize);
			gpuHandle.Offset(1, mCbvSrvUavDescriptorSize);
		}
	}
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mPointShadowTexCPUHandleStart);
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mPointShadowTexGPUHandleStart);
		for (UINT i = 0; i < mPointLights.size(); i++) {
			auto& pointLight = mPointLights[i];
			pointLight.ShadowSRVID = i;
			pointLight.ShadowRT->SetSRVCPUHandle(cpuHandle);
			pointLight.ShadowRT->SetSRVGPUHandle(gpuHandle);
			cpuHandle.Offset(1, mCbvSrvUavDescriptorSize);
			gpuHandle.Offset(1, mCbvSrvUavDescriptorSize);
		}
	}
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mDirShadowTexCPUHandleStart);
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mDirShadowTexGPUHandleStart);
		for (UINT i = 0; i < mDirLights.size(); i++) {
			auto& dirLight = mDirLights[i];
			dirLight.ShadowSRVID = i;
			dirLight.ShadowRT->SetSRVCPUHandle(cpuHandle);
			dirLight.ShadowRT->SetSRVGPUHandle(gpuHandle);
			cpuHandle.Offset(1, mCbvSrvUavDescriptorSize);
			gpuHandle.Offset(1, mCbvSrvUavDescriptorSize);
		}
	}
}

void SceneGraphApp::BuildInputLayout() {
	{
	/*
		Vertex {
			float3 pos;
			float3 normal;
			float3 tangent;
			float2 tex;
		}
	*/
		// Describe vertex input element
		D3D12_INPUT_ELEMENT_DESC pos;
		pos.SemanticName = "POSITION";
		pos.SemanticIndex = 0;
		pos.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		pos.AlignedByteOffset = 0;
		pos.InputSlot = 0;
		pos.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		pos.InstanceDataStepRate = 0;

		auto normal = pos;
		normal.SemanticName = "NORMAL";
		normal.AlignedByteOffset += sizeof(XMFLOAT3);

		auto tangent = normal;
		tangent.SemanticName = "TANGENT";
		tangent.AlignedByteOffset += sizeof(XMFLOAT3);

		auto tex = tangent;
		tex.Format = DXGI_FORMAT_R32G32_FLOAT;
		tex.SemanticName = "TEXTURE";
		tex.AlignedByteOffset += sizeof(XMFLOAT3);

		mInputLayouts["standard"] = { pos, normal, tangent, tex };
	}
	{
	/*
		Vertex {
			float3 pos;
		}
	*/
		D3D12_INPUT_ELEMENT_DESC pos;
		pos.SemanticName = "POSITION";
		pos.SemanticIndex = 0;
		pos.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		pos.AlignedByteOffset = 0;
		pos.InputSlot = 0;
		pos.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		pos.InstanceDataStepRate = 0;

		mInputLayouts["onlyPos"] = { pos };
	}
}

ComPtr<ID3D12RootSignature> SerializeAndCreateRootSignature(ComPtr<ID3D12Device> device, D3D12_ROOT_SIGNATURE_DESC* rootSignDesc) {
	// Serialize
	ComPtr<ID3DBlob> serializedRootSignBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	auto hr = D3D12SerializeRootSignature(
		rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignBlob.GetAddressOf(), errorBlob.GetAddressOf()
	);

	// Check if failed
	if (errorBlob != nullptr)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	// Create root signature
	ComPtr<ID3D12RootSignature> rootSignature;
	ThrowIfFailed(device->CreateRootSignature(
		0, 
		serializedRootSignBlob->GetBufferPointer(),
		serializedRootSignBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	));
	return rootSignature;
}

void SceneGraphApp::BuildRootSignature()
{
	auto GetCBVParam = [](UINT shaderRegister){
		CD3DX12_ROOT_PARAMETER param;
		param.InitAsConstantBufferView(shaderRegister);
		return param;
	};
	auto GetTableParam = [](const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges) {
		CD3DX12_ROOT_PARAMETER param;
		param.InitAsDescriptorTable(
			static_cast<UINT>(ranges.size()),
			ranges.data()
		);
		return param;
	};

	auto staticSamplers = GetStaticSamplers();

	// Shadow
	{
	/*
		cb perObject;
		cb perPass;
	*/
		// Describe root parameters
		UINT index = 0;
		std::unordered_map<std::string, UINT> paramIndices;
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetCBVParam(0)); // 0
		paramIndices["objectCB"] = index++;
		rootParams.push_back(GetCBVParam(1)); // 1
		paramIndices["passCB"] = index++;

		// Create desc for root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
		rootSignDesc.Init(
			static_cast<UINT>(rootParams.size()), 
			rootParams.data(),
			static_cast<UINT>(staticSamplers.size()),
			staticSamplers.data()
		);
		rootSignDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize And Create RootSignature
		mRootSigns["shadow"] = SerializeAndCreateRootSignature(md3dDevice, &rootSignDesc);
		mRootSignParamIndices["shadow"] = paramIndices;
	}

	// Standard
	{
	/*
		cb perObject;
		cb perMaterial;
		cb perPass;

		uab<uint32> ncount;
		srb zbuffer;
		srb textures[];
		srb spotShadows[];
		srb dirShadows[];
	*/
		std::vector<D3D12_DESCRIPTOR_RANGE> nCountUAVranges;
		nCountUAVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
			1, 0,
			0, 0U
		));
		std::vector<D3D12_DESCRIPTOR_RANGE> zbufferSRVranges;
		zbufferSRVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
			1, 0,
			0, 0U
		));
		std::vector<D3D12_DESCRIPTOR_RANGE> texSRVranges;
		if (mTextures.size()) {
			texSRVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				(UINT)mTextures.size(),
				1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			));
		}
		std::vector<D3D12_DESCRIPTOR_RANGE> spotShadowSRVranges;
		if (mSpotLights.size()) {
			spotShadowSRVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				(UINT)mSpotLights.size(),
				0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			));
		}
		std::vector<D3D12_DESCRIPTOR_RANGE> dirShadowSRVranges;
		if (mDirLights.size()) {
			dirShadowSRVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				(UINT)mDirLights.size(),
				0, 2, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			));
		}
		std::vector<D3D12_DESCRIPTOR_RANGE> pointShadowSRVranges;
		if (mPointLights.size()) {
			pointShadowSRVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				(UINT)mPointLights.size(),
				0, 3, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			));
		}

		UINT index = 0;
		std::unordered_map<std::string, UINT> paramIndices;
		// Describe root parameters
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetCBVParam(0)); // 0
		paramIndices["objectCB"] = index++;
		rootParams.push_back(GetCBVParam(1)); // 1
		paramIndices["materialCB"] = index++;
		rootParams.push_back(GetCBVParam(2)); // 2
		paramIndices["passCB"] = index++;
		rootParams.push_back(GetTableParam(nCountUAVranges)); // 3
		paramIndices["ncountUA"] = index++;
		rootParams.push_back(GetTableParam(zbufferSRVranges)); // 5
		paramIndices["zbufferSR"] = index++;
		if (texSRVranges.size()) {
			rootParams.push_back(GetTableParam(texSRVranges)); // 5
			paramIndices["texSR"] = index++;
		}
		if (spotShadowSRVranges.size()) {
			rootParams.push_back(GetTableParam(spotShadowSRVranges)); // 6
			paramIndices["spotShadowSR"] = index++;
		}
		if (dirShadowSRVranges.size()) {
			rootParams.push_back(GetTableParam(dirShadowSRVranges)); // 7
			paramIndices["dirShadowSR"] = index++;
		}
		if (pointShadowSRVranges.size()) {
			rootParams.push_back(GetTableParam(pointShadowSRVranges)); // 8
			paramIndices["pointShadowSR"] = index++;
		}

		// Create desc for root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
		rootSignDesc.Init(
			static_cast<UINT>(rootParams.size()),
			rootParams.data(),
			static_cast<UINT>(staticSamplers.size()),
			staticSamplers.data()
		);
		rootSignDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize And Create RootSignature
		mRootSigns["standard"] = SerializeAndCreateRootSignature(md3dDevice, &rootSignDesc);
		mRootSignParamIndices["standard"] = paramIndices;
	}

	/* Not using
	// HBAO
	{
		std::vector<D3D12_DESCRIPTOR_RANGE> depthSRVranges;
		depthSRVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
			1, 0,
			0, 0U
		));
		std::vector<D3D12_DESCRIPTOR_RANGE> colorSRVranges;
		colorSRVranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
			1, 1,
			0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		));

		// Describe root parameters
		UINT index = 0;
		std::unordered_map<std::string, UINT> paramIndices;
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetCBVParam(0)); // 0
		paramIndices["hbaoCB"] = index++;
		rootParams.push_back(GetTableParam(depthSRVranges)); // 3
		paramIndices["depthSRV"] = index++;
		rootParams.push_back(GetTableParam(colorSRVranges)); // 4
		paramIndices["colorSRV"] = index++;

		// Create desc for root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
		rootSignDesc.Init(
			static_cast<UINT>(rootParams.size()),
			rootParams.data(),
			static_cast<UINT>(staticSamplers.size()),
			staticSamplers.data()
		);
		rootSignDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize And Create RootSignature
		mRootSigns["hbao"] = SerializeAndCreateRootSignature(md3dDevice, &rootSignDesc);
		mRootSignParamIndices["hbao"] = paramIndices;
	}
	*/

	/* Not using
	// transBlend
	{ 
		// Descriptor ranges
		std::vector<D3D12_DESCRIPTOR_RANGE> opaqueSRVRanges;
		opaqueSRVRanges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
			1, 0, 
			0, 0U
		));
		std::vector<D3D12_DESCRIPTOR_RANGE> transSRVRanges;
		transSRVRanges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
			1, 1, 
			0, 0U
		));
		std::vector<D3D12_DESCRIPTOR_RANGE> UAVRanges;
		UAVRanges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
			1, 0,
			0, 0U
		));

		// Describe root parameters
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetTableParam(opaqueSRVRanges));
		rootParams.push_back(GetTableParam(transSRVRanges));
		rootParams.push_back(GetTableParam(UAVRanges));

		// Create desc for root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
		rootSignDesc.Init(static_cast<UINT>(rootParams.size()), rootParams.data());
		rootSignDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize And Create RootSignature
		mRootSigns["transBlend"] = SerializeAndCreateRootSignature(md3dDevice, &rootSignDesc);
	}
	*/

	// FXAA
	{ 
	/*
		cb fxaa;
		tex tex;
	*/
		// Descriptor Ranges
		std::vector<D3D12_DESCRIPTOR_RANGE> texSRVRanges;
		texSRVRanges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
			1, 0, 
			0, 0U
		));

		// Describe root parametes
		int index = 0;
		std::unordered_map<std::string, UINT> paramIndices;
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetCBVParam(0));
		paramIndices["passCB"] = index++;
		rootParams.push_back(GetTableParam(texSRVRanges));
		paramIndices["texSR"] = index++;

		// Create desc for root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
		rootSignDesc.Init(
			static_cast<UINT>(rootParams.size()),
			rootParams.data(),
			static_cast<UINT>(staticSamplers.size()),
			staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		// Serialize And Create RootSignature
		mRootSigns["fxaa"] = SerializeAndCreateRootSignature(md3dDevice, &rootSignDesc);
		mRootSignParamIndices["fxaa"] = paramIndices;
	}
}

void SceneGraphApp::BuildShaders()
{
	static const std::string VS_TARGET = "vs_5_1";
	static const std::string HS_TARGET = "hs_5_1";
	static const std::string DS_TARGET = "ds_5_1";
	static const std::string PS_TARGET = "ps_5_1";

	// defines
	std::vector<D3D_SHADER_MACRO> defines;
	if(m4xMsaaState)
		defines.push_back({ "MULTIPLE_SAMPLE", "4" });
	std::string maxDirLightNumStr = std::to_string(MAX_DIR_LIGHT_NUM);
	std::string maxPointLightNumStr = std::to_string(MAX_POINT_LIGHT_NUM);
	std::string maxSpotLightNumStr = std::to_string(MAX_SPOT_LIGHT_NUM);
	std::string maxRectLightNumStr = std::to_string(MAX_RECT_LIGHT_NUM);
	std::string textureNumStr = std::to_string(mTextures.size());
	std::string spotShadowTexNumStr = std::to_string(mSpotLights.size());
	std::string dirShadowTexNumStr = std::to_string(mDirLights.size());
	std::string pointShadowTexNumStr = std::to_string(mPointLights.size());
	defines.push_back({ "MAX_DIR_LIGHT_NUM", maxDirLightNumStr.c_str() });
	defines.push_back({ "MAX_POINT_LIGHT_NUM", maxPointLightNumStr.c_str() });
	defines.push_back({ "MAX_SPOT_LIGHT_NUM", maxSpotLightNumStr.c_str() });
	defines.push_back({ "MAX_RECT_LIGHT_NUM", maxRectLightNumStr.c_str() });
	defines.push_back({ "TEXTURE_NUM",  textureNumStr.c_str()});
	defines.push_back({ "SPOT_SHADOW_TEX_NUM",  spotShadowTexNumStr.c_str()});
	defines.push_back({ "DIR_SHADOW_TEX_NUM",  dirShadowTexNumStr.c_str()});
	defines.push_back({ "POINT_SHADOW_TEX_NUM",  pointShadowTexNumStr.c_str()});
	defines.push_back({ NULL, NULL });

	// compile
	mShaders["vs"] = d3dUtil::CompileShader(
		L"VertexShader.hlsl", defines.data(), "main", VS_TARGET
	);
	mShaders["ps"] = d3dUtil::CompileShader(
		L"PixelShader.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["simpleVS"] = d3dUtil::CompileShader(
		L"simpleVertex.hlsl", defines.data(), "main", VS_TARGET
	);
	mShaders["fxaaPS"] = d3dUtil::CompileShader(
		L"fxaaPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["dispHS"] = d3dUtil::CompileShader(
		L"displacementHull.hlsl", defines.data(), "main", HS_TARGET
	);
	mShaders["dispDS"] = d3dUtil::CompileShader(
		L"displacementDomain.hlsl", defines.data(), "main", DS_TARGET
	);
	mShaders["shadowVS"] = d3dUtil::CompileShader(
		L"shadowVertex.hlsl", defines.data(), "main", VS_TARGET
	);
	mShaders["shadowPS"] = d3dUtil::CompileShader(
		L"shadowPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["pointShadowVS"] = d3dUtil::CompileShader(
		L"pointShadowVertex.hlsl", defines.data(), "main", VS_TARGET
	);
	mShaders["pointShadowPS"] = d3dUtil::CompileShader(
		L"pointShadowPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	/*
	mShaders["transPS"] = d3dUtil::CompileShader(
		L"transPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["transBlendPS"] = d3dUtil::CompileShader(
		L"transBlendPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["hbaoPS"] = d3dUtil::CompileShader(
		L"hbaoPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	*/
}

void SceneGraphApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { 
		mInputLayouts["standard"].data(),
		(UINT)mInputLayouts["standard"].size() 
	};
	opaquePsoDesc.pRootSignature = mRootSigns["standard"].Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["vs"]->GetBufferPointer()),
		mShaders["vs"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ps"]->GetBufferPointer()),
		mShaders["ps"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mRenderTargets["opaque"]->GetColorViewFormat();
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mRenderTargets["opaque"]->GetDepthStencilViewFormat();
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC dispOpaquePsoDesc = opaquePsoDesc;
	dispOpaquePsoDesc.HS =
	{
		reinterpret_cast<BYTE*>(mShaders["dispHS"]->GetBufferPointer()),
		mShaders["dispHS"]->GetBufferSize()
	};
	dispOpaquePsoDesc.DS =
	{
		reinterpret_cast<BYTE*>(mShaders["dispDS"]->GetBufferPointer()),
		mShaders["dispDS"]->GetBufferSize()
	};
	dispOpaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&dispOpaquePsoDesc, IID_PPV_ARGS(&mPSOs["dispOpaque"])));

	/*
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transPsoDesc = opaquePsoDesc;
	transPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["transPS"]->GetBufferPointer()),
		mShaders["transPS"]->GetBufferSize()
	};
	transPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	transPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	transPsoDesc.BlendState.RenderTarget[0].BlendEnable = true;
	transPsoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	transPsoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_DEST_COLOR;
	transPsoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_COLOR;
	transPsoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transPsoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
	transPsoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	transPsoDesc.RTVFormats[0] = mRenderTargets["trans"]->GetColorViewFormat();
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transPsoDesc, IID_PPV_ARGS(&mPSOs["trans"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transBlendPsoDesc = opaquePsoDesc;
	transBlendPsoDesc.InputLayout = { 
		mInputLayouts["onlyPos"].data(), 
		(UINT)mInputLayouts["onlyPos"].size() 
	};
	transBlendPsoDesc.pRootSignature = mRootSigns["transBlend"].Get();
	transBlendPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["simpleVS"]->GetBufferPointer()),
		mShaders["simpleVS"]->GetBufferSize()
	};
	transBlendPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["transBlendPS"]->GetBufferPointer()),
		mShaders["transBlendPS"]->GetBufferSize()
	};
	transBlendPsoDesc.DepthStencilState.DepthEnable = false;
	transBlendPsoDesc.RTVFormats[0] = mRenderTargets["transBlend"]->GetColorViewFormat();
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transBlendPsoDesc, IID_PPV_ARGS(&mPSOs["transBlend"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC hbaoPsoDesc = transBlendPsoDesc;
	hbaoPsoDesc.pRootSignature = mRootSigns["hbao"].Get();
	hbaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["hbaoPS"]->GetBufferPointer()),
		mShaders["hbaoPS"]->GetBufferSize()
	};
	hbaoPsoDesc.RTVFormats[0] = mRenderTargets["hbao"]->GetColorViewFormat();
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&hbaoPsoDesc, IID_PPV_ARGS(&mPSOs["hbao"])));
	*/

	D3D12_GRAPHICS_PIPELINE_STATE_DESC fxaaPsoDesc = opaquePsoDesc;
	fxaaPsoDesc.InputLayout = { 
		mInputLayouts["onlyPos"].data(), 
		(UINT)mInputLayouts["onlyPos"].size() 
	};
	fxaaPsoDesc.pRootSignature = mRootSigns["fxaa"].Get();
	fxaaPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["simpleVS"]->GetBufferPointer()),
		mShaders["simpleVS"]->GetBufferSize()
	};
	fxaaPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["fxaaPS"]->GetBufferPointer()),
		mShaders["fxaaPS"]->GetBufferSize()
	};
	fxaaPsoDesc.DepthStencilState.DepthEnable = false;
	fxaaPsoDesc.RTVFormats[0] = mRenderTargets["fxaa"]->GetColorViewFormat();
	fxaaPsoDesc.SampleDesc.Count = 1;
	fxaaPsoDesc.SampleDesc.Quality = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&fxaaPsoDesc, IID_PPV_ARGS(&mPSOs["fxaa"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = opaquePsoDesc;
	shadowPsoDesc.InputLayout = {
		mInputLayouts["onlyPos"].data(),
		(UINT)mInputLayouts["onlyPos"].size()
	};
	shadowPsoDesc.pRootSignature = mRootSigns["shadow"].Get();
	shadowPsoDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
		mShaders["shadowVS"]->GetBufferSize()
	};
	shadowPsoDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["shadowPS"]->GetBufferPointer()),
		mShaders["shadowPS"]->GetBufferSize()
	};
	shadowPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	shadowPsoDesc.RTVFormats[0] = SHADOW_RTV_FORMAT;
	shadowPsoDesc.SampleDesc.Count = 1;
	shadowPsoDesc.SampleDesc.Quality = 0;
	shadowPsoDesc.DSVFormat = SHADOW_DSV_VIEW_FORMAT;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs["shadow"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pointShadowPsoDesc = shadowPsoDesc;
	pointShadowPsoDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["pointShadowVS"]->GetBufferPointer()),
		mShaders["pointShadowVS"]->GetBufferSize()
	};
	pointShadowPsoDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["pointShadowPS"]->GetBufferPointer()),
		mShaders["pointShadowPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&pointShadowPsoDesc, IID_PPV_ARGS(&mPSOs["pointShadow"])));
}

void SceneGraphApp::LoadTextures()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mTexCPUHandleStart);
	for (UINT i = 0; i < Texture::GetTotalNum(); i++) {
		auto tex = Texture::FindByID(i);
		tex->LoadAndCreateSRV(md3dDevice, mCommandList, handle);
		handle.Offset(mCbvSrvUavDescriptorSize);
	}
}

void SceneGraphApp::BuildPassConstants()
{
	mPassConstants = std::make_unique<PassConstants>();
}

void SceneGraphApp::BuildPassConstantBuffers()
{
	mPassConstantsBuffers = std::make_unique<UploadBuffer<PassConstants::Content>>(
		md3dDevice.Get(), 
		PassConstants::getTotalNum(), true
	);
}

void SceneGraphApp::UpdateLightsInPassConstantBuffers()
{
	// Check light num
	if(mDirLights.size() > MAX_DIR_LIGHT_NUM)
		throw "DirLights too many with " + std::to_string(mDirLights.size());
	if(mPointLights.size() > MAX_POINT_LIGHT_NUM)
		throw "PointLights too many with " + std::to_string(mPointLights.size());
	if(mSpotLights.size() > MAX_SPOT_LIGHT_NUM)
		throw "SpotLights too many with " + std::to_string(mSpotLights.size());
	if(mRectLights.size() > MAX_RECT_LIGHT_NUM)
		throw "RectLights too many with " + std::to_string(mRectLights.size());

	auto& content = mPassConstants->content;

	// direction lights
	content.LightPerTypeNum.x = static_cast<UINT32>(mDirLights.size());

	// point lights
	for (unsigned int i = 0; i < mPointLights.size(); i++)
		content.PointLights[i] = mPointLights[i].ToContent();
	content.LightPerTypeNum.y = static_cast<UINT32>(mPointLights.size());

	// spot lights
	for (unsigned int i = 0; i < mSpotLights.size(); i++)
		content.SpotLights[i] = mSpotLights[i].ToContent();
	content.LightPerTypeNum.z = static_cast<UINT32>(mSpotLights.size());

	// area lights
	for (unsigned int i = 0; i < mRectLights.size(); i++)
		content.RectLights[i] = mRectLights[i].ToContent();
	content.LightPerTypeNum.w = static_cast<UINT32>(mRectLights.size());
}

/*
void SceneGraphApp::BuildGeos()
{
	{
		struct Vertex_in {
			XMFLOAT3 pos;
			XMFLOAT3 normal;
			XMFLOAT3 tangent;
			XMFLOAT2 tex;
		};

		// Create Geo
		auto geo = std::make_shared<MeshGeometry>();
		geo->Name = "triangle";

		// Generate Vertex Info
		GeometryGenerator geoGenerator;
		std::vector<UINT32> indices;
		std::vector<Vertex_in> verts;
		{
			// Box
			GeometryGenerator::MeshData boxMesh = geoGenerator.CreateBox(1.0, 1.0, 1.0, 1);
			for (const auto& vert : boxMesh.Vertices) {
				verts.push_back({ vert.Position, vert.Normal, vert.TangentU, vert.TexC });
			}
			indices = boxMesh.Indices32;

			SubmeshGeometry submesh;
			submesh.BaseVertexLocation = 0;
			submesh.StartIndexLocation = 0;
			submesh.IndexCount = static_cast<UINT>(indices.size());
			geo->DrawArgs["triangle"] = submesh;
		}
		{
			// Board
			std::vector<XMFLOAT3> boardVerts = {
				{-0.8f, -0.8f, 0.0f},
				{-0.8f, +0.8f, 0.0f},
				{+0.8f, -0.8f, 0.0f},
				{+0.8f, +0.8f, 0.0f},
			};
			std::vector<XMFLOAT2> boardUVs = {
				{0.0f, 1.0f},
				{0.0f, 0.0f},
				{1.0f, 1.0f},
				{1.0f, 0.0f}
			};
			std::vector<UINT32> boardIndices = {
				0, 1, 2,
				1, 3, 2,
			};

			SubmeshGeometry submesh;
			submesh.BaseVertexLocation = static_cast<UINT>(verts.size());
			submesh.StartIndexLocation = static_cast<UINT>(indices.size());
			submesh.IndexCount = static_cast<UINT>(boardIndices.size());
			geo->DrawArgs["board"] = submesh;

			for (UINT i = 0; i < boardVerts.size(); i++) {
				verts.push_back({ 
					boardVerts[i], 
					{0.0f, 0.0f, -1.0f}, 
					{1.0f, 0.0f, 0.0f},
					boardUVs[i] 
				});
			}
			indices.insert(indices.end(), boardIndices.begin(), boardIndices.end());
		}
		{
			// ViewVolume
			std::vector<XMFLOAT3> subVerts = {
				{-0.5f, -0.5f, -0.5f}, // left down back // 0
				{0.5f, -0.5f, -0.5f}, // right down back // 1
				{0.5f, 0.5f, -0.5f}, // right up back // 2
				{-0.5f, 0.5f, -0.5f}, // left up back // 3
				{-1.0f, -1.0f, 0.5f}, // left down front // 4
				{1.0f, -1.0f, 0.5f}, // right down front // 5
				{1.0f, 1.0f, 0.5f}, // right up front // 6
				{-1.0f, 1.0f, 0.5f} // left up front // 7
			};
			std::vector<XMFLOAT3> subNormals = subVerts;
			for (auto& n : subNormals) {
				XMVECTOR k = XMLoadFloat3(&n);
				k = XMVector3Normalize(k);
				XMStoreFloat3(&n, k);
			}
			std::vector<UINT32> subIndices = {
				0, 3, 1, // down in back
				2, 1, 3, // up in back

				4, 5, 7, // down in front
				6, 7, 5, // up in front

				3, 0, 7, // up in left
				7, 0, 4, // down in left

				2, 6, 1, // up in right
				1, 6, 5, // down in right

				7, 6, 3, // left in up
				3, 6, 2, // right in up

				5, 4, 0, // left in down
				0, 1, 5 // right in down
			};

			SubmeshGeometry submesh;
			submesh.BaseVertexLocation = static_cast<UINT>(verts.size());
			submesh.StartIndexLocation = static_cast<UINT>(indices.size());
			submesh.IndexCount = static_cast<UINT>(subIndices.size());
			geo->DrawArgs["viewVolume"] = submesh;

			for (UINT i = 0; i < subVerts.size(); i++) {
				verts.push_back({ 
					subVerts[i], 
					subNormals[i]
				});
			}
			indices.insert(indices.end(), subIndices.begin(), subIndices.end());
		}

		// Fill buffer info & Upload
		FillBufferInfoAndUpload(
			md3dDevice, mCommandList,
			geo, 
			verts, indices,
			DXGI_FORMAT_R32_UINT
		);

		// Save geo
		mGeos[geo->Name] = std::move(geo);
	}

}
*/

void SceneGraphApp::BuildManualMaterials()
{
	auto defaultMtl = std::make_shared<Material>("default");
	defaultMtl->mBaseColor = { 0.972f, 0.960f, 0.915f, 1.0f }; // silver
	defaultMtl->mMetalness = 1.0f;
	defaultMtl->mRoughness = 0.5f;
	defaultMtl->mLTCMatTexID = Texture::FindByName("ggx_ltc_mat")->GetID();
	defaultMtl->mLTCAmpTexID = Texture::FindByName("ggx_ltc_amp")->GetID();
	mMaterials.push_back(defaultMtl);
	Material::SetDefaultMaterialID(defaultMtl->GetID());

/*
	auto whiteMtl = std::make_shared<MaterialConstants>();
	// whiteMtl->content.Diffuse = { 0.9f, 0.8f, 0.9f, 1.0f };
	// whiteMtl->content.Specular = { 0.04f, 0.04f, 0.04f, 0.04f }; // default dielectrics
	whiteMtl->content.Diffuse = { 0.0f, 0.0f, 0.0f, 0.0f };
	whiteMtl->content.Specular = { 0.972f, 0.960f, 0.915f, 1.0f }; // silver
	whiteMtl->content.LTCMatTexID = mResourceTextures["ggx_ltc_mat"]->ID + 1;
	whiteMtl->content.LTCAmpTexID = mResourceTextures["ggx_ltc_amp"]->ID + 1;
	// whiteMtl->content.DiffuseTexID = mResourceTextures["color"]->ID + 1;
	// whiteMtl->content.DispTexID = mResourceTextures["height"]->ID + 1;
	// whiteMtl->content.DispHeightScale = 0.7f;
	// whiteMtl->content.NormalTexID = mResourceTextures["normal"]->ID + 1;
	mMtlConsts["white"] = whiteMtl;
*/
/*
	auto blueMtl = std::make_shared<MaterialConstants>();
	blueMtl->content.Diffuse = { 0.0f, 0.0f, 1.0f, 1.0f };
	mMtlConsts["blue"] = blueMtl;

	auto redMtl = std::make_shared<MaterialConstants>();
	redMtl->content.Diffuse = { 1.0f, 0.0f, 0.0f, 1.0f };
	mMtlConsts["red"] = redMtl;
	
	auto transBlueMtl = std::make_shared<MaterialConstants>();
	transBlueMtl->content.Diffuse = { 0.0f, 0.0f, 0.8f, 0.7f };
	mMtlConsts["transBlue"] = transBlueMtl;

	auto transRedMtl = std::make_shared<MaterialConstants>();
	transRedMtl->content.Diffuse = { 0.8f, 0.0f, 0.0f, 0.7f };
	mMtlConsts["transRed"] = transRedMtl;

	auto cutoutTreeMtl = std::make_shared<MaterialConstants>();
	cutoutTreeMtl->content.DiffuseTexID = mResourceTextures["tree"]->ID + 1;
	cutoutTreeMtl->content.AlphaTestTheta = 0.2f;
	mMtlConsts["cutoutTree"] = cutoutTreeMtl;
*/
}

void SceneGraphApp::BuildAndUpdateMaterialConstantBuffers()
{
	// Build Buffers
	mMaterialConstantsBuffers = std::make_unique<UploadBuffer<Material::Content>>(
		md3dDevice.Get(), 
		Material::GetTotalNum(), true
	);
	
	// Update Buffers
	for (UINT id = 0; id < Material::GetTotalNum(); id++) {
		auto mtl = Material::FindObjectByID(id);
		mMaterialConstantsBuffers->CopyData(
			id,
			mtl->ToContent()
		);
	}
}

/*
void SceneGraphApp::BuildRenderItems()
{
	// Test
	{
		// blue
		{
			// Create Object constants
			auto consts = std::make_shared<ObjectConstants>();
			auto translationMat = XMMatrixTranslation(0.0f, 0.0f, -0.8f);
			XMStoreFloat4x4(
				&consts->content.ModelMat, 
				XMMatrixTranspose(translationMat)
			);

			mObjConsts["blueBoard"] = consts;

			// Create render items
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->MeshPool = "triangle"
			renderItem->SubMesh = "board";
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				renderItem->Material = "white";
			renderItem->PSO = "opaque";
			renderItem->ObjectID = mObjConsts["blueBoard"]->getID();

			// Save render items
			mRenderItemQueue.push_back(std::move(renderItem));
		}

		// red
		{
			// Create Object constants
			auto consts = std::make_shared<ObjectConstants>();
			auto translationMat = XMMatrixTranslation(0.0f, 0.0f, 0.8f);
			XMStoreFloat4x4(
				&consts->content.ModelMat, 
				XMMatrixTranspose(translationMat)
			);

			mObjConsts["redBoard"] = consts;

			// Create render items
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->MeshPool = "triangle"
			renderItem->SubMesh = "board";
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				renderItem->Material = "white";
			renderItem->PSO = "opaque";
			renderItem->ObjectID = mObjConsts["redBoard"]->getID();

			// Save render items
			mRenderItemQueue.push_back(std::move(renderItem));
		}
	}

	// Opaque
	{
		// Tree
		{
			// Hor
			{
				auto horConsts = std::make_shared<ObjectConstants>();
				horConsts->content.ModelMat = MathHelper::Identity4x4();
				mObjConsts["horTree"] = horConsts;

				// Create render items
				auto horItem = std::make_shared<RenderItem>();
			horItem->MeshPool = "triangle"
			horItem->SubMesh = "board";
				horItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				renderItem->Material = "white";
				horItem->PSO = "opaque";
				horItem->ObjectID = mObjConsts["horTree"]->getID();

				// Save render items
				mOpaqueRenderItemQueue.push_back(std::move(horItem));
			}

			// Ver
			{
				auto verConsts = std::make_shared<ObjectConstants>();
				auto modelMat = XMMatrixRotationY(MathHelper::AngleToRadius(90));
				XMStoreFloat4x4(
					&verConsts->content.ModelMat, 
					XMMatrixTranspose(modelMat)
				);
				mObjConsts["verTree"] = verConsts;

				// Create render items
				auto verItem = std::make_shared<RenderItem>();
			verItem->MeshPool = "triangle"
			verItem->SubMesh = "board";
				verItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				renderItem->Material = "white";
				verItem->PSO = "opaque";
				verItem->ObjectID = mObjConsts["verTree"]->getID();

				// Save render items
				mOpaqueRenderItemQueue.push_back(std::move(verItem));
			}

		}


		// Displacement Cube
		{
			// Create Object constants
			auto triConsts = std::make_shared<ObjectConstants>();
			triConsts->content.ModelMat = MathHelper::Identity4x4();
			mObjConsts["triangle"] = triConsts;

			// Create render items
			auto renderItem = std::make_shared<RenderItem>();
				renderItem->MeshPool = "triangle";
				renderItem->SubMesh = "triangle";
			renderItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
				renderItem->Material = "white";
			renderItem->PSO = "dispOpaque";
				renderItem->ObjectID = mObjConsts["triangle"]->getID();

			// Save render items
			mRenderItemQueue.push_back(std::move(renderItem));
		}

	}

	// Transparent
	{
		// transBlue
		{
			// Create Object constants
			auto consts = std::make_shared<ObjectConstants>();
			auto translationMat = XMMatrixTranslation(0.0f, 0.0f, -0.8f);
			XMStoreFloat4x4(
				&consts->content.ModelMat, 
				XMMatrixTranspose(translationMat)
			);
			mObjConsts["transBlueBoard"] = consts;

			// Create render items
			auto renderItem = std::make_shared<RenderItem>();
				renderItem->MeshPool = "triangle";
				renderItem->SubMesh = "board";
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				renderItem->Material = "white";
			renderItem->PSO = "trans";
				renderItem->ObjectID = mObjConsts["transBlueBoard"]->getID();

			// Save render items
			mTransRenderItemQueue.push_back(std::move(renderItem));
		}

		// transRed
		{
			// Create Object constants
			auto consts = std::make_shared<ObjectConstants>();
			auto translationMat = XMMatrixTranslation(0.0f, 0.0f, 0.8f);
			XMStoreFloat4x4(
				&consts->content.ModelMat, 
				XMMatrixTranspose(translationMat)
			);
			mObjConsts["transRedBoard"] = consts;

			// Create render items
			auto renderItem = std::make_shared<RenderItem>();
				renderItem->MeshPool = "triangle";
				renderItem->SubMesh = "board";
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				renderItem->Material = "white";
			renderItem->PSO = "trans";
				renderItem->ObjectID = mObjConsts["transRedBoard"]->getID();

			// Save render items
			mTransRenderItemQueue.push_back(std::move(renderItem));
		}
	}
}
*/

void SceneGraphApp::BuildObjectConstantBuffers()
{
	mObjectConstantsBuffers = std::make_unique<UploadBuffer<Object::Content>>(
		md3dDevice.Get(), 
		Object::GetTotalNum(), true
	);
}

void SceneGraphApp::InitHbao()
{
	// Create constants
	mHbaoConstants = std::make_unique<HbaoConstants>();

	// Init constants
	auto& content = mHbaoConstants->content;
	// TODO now is empty

	// Create buffer
	mHbaoConstantsBuffers = std::make_unique<UploadBuffer<HbaoConstants::Content>>(
		md3dDevice.Get(), 
		HbaoConstants::getTotalNum(), true
	);
}

void SceneGraphApp::InitFxaa()
{
	// Settings
	constexpr float qualitySubpix = 0.75f;
	constexpr float qualityEdgeThreshold = 0.166f;
	constexpr float qualityEdgeThresholdMin = 0.0833f;
	constexpr float consoleEdgeSharpness = 8.0f;
	constexpr float consoleEdgeThreshold = 0.125f;
	constexpr float consoleEdgeThresholdMin = 0.05f;

	// Create constants
	mFxaaConstants = std::make_unique<FxaaConstants>();

	// Init constants
	auto& content = mFxaaConstants->content;
	content.QualitySubpix = qualitySubpix;
	content.QualityEdgeThreshold = qualityEdgeThreshold;
	content.QualityEdgeThresholdMin = qualityEdgeThresholdMin;
	content.ConsoleEdgeSharpness = consoleEdgeSharpness;
	content.ConsoleEdgeThreshold = consoleEdgeThreshold;
	content.ConsoleEdgeThresholdMin = consoleEdgeThresholdMin;
	content.Console360ConstDir = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Create buffer
	mFxaaConstantsBuffers = std::make_unique<UploadBuffer<FxaaConstants::Content>>(
		md3dDevice.Get(), 
		FxaaConstants::getTotalNum(), true
	);
}

void SceneGraphApp::ResizeScreenUAVSRV()
{
	// UAVs
	{
		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = m4xMsaaState ? 4*mClientWidth : mClientWidth;
		desc.Height = mClientHeight;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = mUABs["nCount"]->GetViewFormat();
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		mUABs["nCount"]->Resize(md3dDevice, &desc);
	}

	// ZBuffer SRV
	{
		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = mClientWidth;
		desc.Height = mClientHeight;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = mZBufferResourceFormat;
		desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
		desc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(mZBufferResource.GetAddressOf())));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = mZBufferViewFormat;
		srvDesc.ViewDimension = m4xMsaaState ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		md3dDevice->CreateShaderResourceView(
			mZBufferResource.Get(),
			&srvDesc, mZBufferSRVCPUHandle
		);
	}
}

void SceneGraphApp::ResizeRenderTargets()
{
	// Opaque Rendering
	D3D12_RESOURCE_DESC opaqueDesc;
	opaqueDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	opaqueDesc.Alignment = 0;
	opaqueDesc.Width = mClientWidth;
	opaqueDesc.Height = mClientHeight;
	opaqueDesc.DepthOrArraySize = 1;
	opaqueDesc.MipLevels = 1;
	opaqueDesc.Format = mRenderTargets["opaque"]->GetColorViewFormat();
	opaqueDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaqueDesc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality-1 : 0;
	opaqueDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	opaqueDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_RESOURCE_DESC opaqueDSDesc = opaqueDesc;
	opaqueDSDesc.Format = mRenderTargets["opaque"]->GetDepthStencilResourceFormat();
    opaqueDSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	mRenderTargets["opaque"]->Resize(
		md3dDevice,
		&opaqueDesc,
		&opaqueDSDesc
	);

	/*
	// Transparent Rendering
	D3D12_RESOURCE_DESC transDesc = opaqueDesc;
	transDesc.Format = mRenderTargets["trans"]->GetColorViewFormat();

	mRenderTargets["trans"]->Resize(
		md3dDevice,
		&transDesc
	);

	// Transparent Blending
	D3D12_RESOURCE_DESC transBlendDesc = opaqueDesc;
	transBlendDesc.Format = mRenderTargets["transBlend"]->GetColorViewFormat();

	mRenderTargets["transBlend"]->Resize(
		md3dDevice,
		&transBlendDesc
	);

	// HBAO
	D3D12_RESOURCE_DESC hbaoDesc = transBlendDesc;
	hbaoDesc.Format = mRenderTargets["hbao"]->GetColorViewFormat();

	mRenderTargets["hbao"]->Resize(
		md3dDevice,
		&hbaoDesc
	);
	*/

	// After Resolve
	D3D12_RESOURCE_DESC afterResolveDesc = opaqueDesc;
	afterResolveDesc.Format = mRenderTargets["afterResolve"]->GetColorViewFormat();
	afterResolveDesc.SampleDesc.Count = 1;
	afterResolveDesc.SampleDesc.Quality = 0;

	mRenderTargets["afterResolve"]->Resize(
		md3dDevice,
		&afterResolveDesc
	);

	// Fxaa
	D3D12_RESOURCE_DESC fxaaDesc = afterResolveDesc;
	fxaaDesc.Format = mRenderTargets["fxaa"]->GetColorViewFormat();

	D3D12_RESOURCE_DESC fxaaDSDesc = opaqueDSDesc;
	fxaaDSDesc.Format = mRenderTargets["fxaa"]->GetDepthStencilResourceFormat();
	fxaaDSDesc.SampleDesc.Count = 1;
    fxaaDSDesc.SampleDesc.Quality = 0;

	mRenderTargets["fxaa"]->Resize(
		md3dDevice,
		&fxaaDesc,
		&fxaaDSDesc
	);

	// Swap Chain
	mSwapChain->Resize(
		md3dDevice,
		mClientWidth, mClientHeight
	);
}

void SceneGraphApp::ResizeShadowRenderTargets() {
	D3D12_RESOURCE_DESC colorDesc;
	colorDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	colorDesc.Alignment = 0;
	colorDesc.Width = SHADOW_MAPPING_WIDTH;
	colorDesc.Height = SHADOW_MAPPING_HEIGHT;
	colorDesc.DepthOrArraySize = 1;
	colorDesc.MipLevels = 1;
	colorDesc.SampleDesc.Count = 1;
	colorDesc.SampleDesc.Quality = 0;
	colorDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_RESOURCE_DESC depthDesc = colorDesc;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	for (auto& dirLight : mDirLights) {
		colorDesc.Format = dirLight.ShadowRT->GetColorViewFormat();
		depthDesc.Format = dirLight.ShadowRT->GetDepthStencilResourceFormat();

		dirLight.ShadowRT->Resize(
			md3dDevice,
			&colorDesc,
			&depthDesc
		);
	}
	for (auto& spotLight : mSpotLights) {
		colorDesc.Format = spotLight.ShadowRT->GetColorViewFormat();
		depthDesc.Format = spotLight.ShadowRT->GetDepthStencilResourceFormat();

		spotLight.ShadowRT->Resize(
			md3dDevice,
			&colorDesc,
			&depthDesc
		);
	}

	for (auto& pointLight : mPointLights) {
		colorDesc.DepthOrArraySize = 6;
		colorDesc.Format = pointLight.ShadowRT->GetColorViewFormat();
		depthDesc.Format = pointLight.ShadowRT->GetDepthStencilResourceFormat();

		pointLight.ShadowRT->Resize(
			md3dDevice,
			&colorDesc,
			&depthDesc
		);
	}
}

void SceneGraphApp::ResizeFxaa()
{
	constexpr float N = 0.5f;

	auto& content = mFxaaConstants->content;
	content.QualityRcpFrame.x = 1.0f / mClientWidth;
	content.QualityRcpFrame.y = 1.0f / mClientHeight;
	content.ConsoleRcpFrameOpt.x = -N / mClientWidth;
	content.ConsoleRcpFrameOpt.y = -N / mClientHeight;
	content.ConsoleRcpFrameOpt.z = N / mClientWidth;
	content.ConsoleRcpFrameOpt.w = N / mClientHeight;
	content.ConsoleRcpFrameOpt2.x = -2.0f / mClientWidth;
	content.ConsoleRcpFrameOpt2.y = -2.0f / mClientHeight;
	content.ConsoleRcpFrameOpt2.z = 2.0f / mClientWidth;
	content.ConsoleRcpFrameOpt2.w = 2.0f / mClientHeight;
	content.Console360RcpFrameOpt2.x = 8.0f / mClientWidth;
	content.Console360RcpFrameOpt2.y = 8.0f / mClientHeight;
	content.Console360RcpFrameOpt2.z = -4.0f / mClientWidth;
	content.Console360RcpFrameOpt2.w = -4.0f / mClientHeight;
}

void SceneGraphApp::OnMsaaStateChange()
{
	D3DApp::OnMsaaStateChange();

	BuildShaders();
	BuildPSOs();
}

void SceneGraphApp::OnFxaaStateChange()
{
	// Do nothing.
}

void SceneGraphApp::OnResize()
{
	D3DApp::OnResize();

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	ResizeScreenUAVSRV();
	ResizeRenderTargets();
	ResizeShadowRenderTargets();
	ResizeFxaa();

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	mShadowScreenViewport = mScreenViewport;
	mShadowScreenViewport.Width = static_cast<float>(SHADOW_MAPPING_WIDTH);
	mShadowScreenViewport.Height = static_cast<float>(SHADOW_MAPPING_HEIGHT);
	mShadowScissorRect = { 0, 0, SHADOW_MAPPING_WIDTH, SHADOW_MAPPING_HEIGHT };
}

void SceneGraphApp::Update(const GameTimer& gt)
{
	// Notice: Be careful, matrix need transpose.

	float screenWidthHeightAspect = static_cast<float>(mClientWidth) / static_cast<float>(mClientHeight);

	// Update Dir Lights' Box
	for (auto& dirLight : mDirLights) {
		dirLight.SetViewVolumes(mCamera.GetViewVolumeVerts(screenWidthHeightAspect));
	}

	// Update Dir Lights' ShadowPassConstants
	for (auto& dirLight : mDirLights) {
		auto& content = dirLight.PassConstants->content;

		XMStoreFloat4x4(
			&content.ViewMat,
			XMMatrixTranspose(dirLight.CalLightViewMat())
		);
		XMStoreFloat4x4(
			&content.ProjMat,
			XMMatrixTranspose(dirLight.CalLightProjMat())
		);
	}

	// Update Pass Constants
	{
		auto& content = mPassConstants->content;

		XMVECTOR eyePos = mCamera.CalEyePos();
		XMStoreFloat4(&content.EyePos, eyePos);

		XMMATRIX viewMat = mCamera.GetViewMatrix();
		XMStoreFloat4x4(&content.ViewMat, XMMatrixTranspose(viewMat));

		XMMATRIX projMat = mCamera.GetOrthoProjMatrix(screenWidthHeightAspect);
		XMStoreFloat4x4(&content.ProjMat, XMMatrixTranspose(projMat));

		// direction lights
		for (unsigned int i = 0; i < mDirLights.size(); i++)
			content.DirLights[i] = mDirLights[i].ToContent();
	}

	// Update Hbao Constant
	{
		auto& content = mHbaoConstants->content;

		XMMATRIX projMat = mCamera.GetOrthoProjMatrix(screenWidthHeightAspect);
		content.ScaleX = 1.0f / projMat.r[0].m128_f32[0];
		content.ScaleY = 1.0f / projMat.r[1].m128_f32[1];
		content.NearZ = 0.0f;
		content.FarZ = 1.0f / projMat.r[2].m128_f32[2];
		content.ClientWidth = mClientWidth;
		content.ClientHeight = mClientHeight;
	}

	// Upload Pass Constant
	{
		mPassConstantsBuffers->CopyData(
			mPassConstants->getID(),
			mPassConstants->content
		);
	}

	// Upload Shadow Pass Constant
	{
		for (auto& dirLight : mDirLights) {
			mShadowPassConstantsBuffers->CopyData(
				dirLight.PassConstants->getID(),
				dirLight.PassConstants->content
			);
		}
		for (auto& pointLight : mPointLights) {
			for (UINT i = 0; i < 6; i++) {
				mShadowPassConstantsBuffers->CopyData(
					pointLight.PassConstantsArray[i]->getID(),
					pointLight.PassConstantsArray[i]->content
				);
			}
		}
		for (auto& spotLight : mSpotLights) {
			mShadowPassConstantsBuffers->CopyData(
				spotLight.PassConstants->getID(),
				spotLight.PassConstants->content
			);
		}
	}

	// Upload Object Constant
	{
		auto buffer = mObjectConstantsBuffers.get();
		std::function<void(std::shared_ptr<Object>)> iterFunc;
		iterFunc = [buffer, &iterFunc](std::shared_ptr<Object> root) {
			buffer->CopyData(
				root->GetID(),
				root->ToContent()
			);
			for (auto& child : root->GetChilds())
				iterFunc(child);
		};

		mRootObject->UpdateGlobalModelMatRecursively();
		iterFunc(mRootObject);
	}

	// Upload Hbao Constant
	{
		mHbaoConstantsBuffers->CopyData(
			mHbaoConstants->getID(),
			mHbaoConstants->content
		);
	}

	// Upload Fxaa Constant
	{
		mFxaaConstantsBuffers->CopyData(
			mFxaaConstants->getID(),
			mFxaaConstants->content
		);
	}
}

std::vector<CD3DX12_STATIC_SAMPLER_DESC> SceneGraphApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC bilinearWrap(
		0,
		D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC nearestBorder(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER
	);

	return {
		bilinearWrap,
		nearestBorder
	};
}

void SceneGraphApp::UpdateFXAAState(bool newState) {
    if(mUseFXAA != newState)
    {
        mUseFXAA = newState;
    }
}
