//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "SceneGraphApp.h"
#include "../Common/GeometryGenerator.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

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

	// Init DirectX
	BuildDescriptorHeaps();

	// Init PSO concerned
	BuildInputLayout();
	BuildRootSignature();
	BuildShaders();
	BuildPSOs();

	// Init Scene
	BuildLights();
	BuildScene();

	// Init Testing
	BuildTesting();
	
	// Init Scene Resources
	BuildPassConstantBuffers();
	UpdateLightsInPassConstantBuffers();
	BuildGeos();
	BuildMaterials();
	BuildAndUpdateMaterialConstantBuffers();

	// Init Render Items
	BuildRenderItems();

	// Init Render Item Resources
	BuildObjectConstantBuffers();

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

void SceneGraphApp::BuildDescriptorHeaps()
{
	// Build Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	mRTVHeap = std::make_unique<StaticDescriptorHeap>(
		md3dDevice, rtvHeapDesc
	);
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	mCBVSRVUAVHeap = std::make_unique<StaticDescriptorHeap>(
		md3dDevice, srvHeapDesc
	);

	// Build Handle
	mMidRTVCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
	UINT index = mCBVSRVUAVHeap->Alloc();
	mMidSRVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
	mMidSRVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
}

void SceneGraphApp::ResizeMidRenderTarget()
{
	mMidRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Build Resource
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = mClientWidth;
	desc.Height = mClientHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = mMidRenderTargetFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = mMidRenderTargetFormat;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&mMidRenderTarget)
	));

	// Build Descriptor
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mMidRenderTargetFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateRenderTargetView(
		mMidRenderTarget.Get(),
		&rtvDesc, mMidRTVCPUHandle
	);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = mMidRenderTargetFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	md3dDevice->CreateShaderResourceView(
		mMidRenderTarget.Get(),
		&srvDesc, mMidSRVCPUHandle
	);
}

void SceneGraphApp::BuildInputLayout() {
	{
	/*
		Vertex {
			float3 pos;
			float3 normal;
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
		pos.SemanticName = "NORMAL";
		pos.AlignedByteOffset = sizeof(XMFLOAT3);

		mInputLayouts["standard"] = { pos, normal };
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

		mInputLayouts["post"] = { pos };
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

	{
	/*
		cb perObject{
			float4x4 modelMat;
		}
		cb perMaterial{
			float4 diffuse;
		}
		cb perPass{
			float4x4 viewMat;
			float4x4 projMat;
		}

		uab<uint8> testUA;
	*/
		D3D12_DESCRIPTOR_RANGE range;
		range.BaseShaderRegister = 0;
		range.NumDescriptors = 1;
		range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		range.OffsetInDescriptorsFromTableStart = 0U;
		range.RegisterSpace = 0;

		// Describe root parameters
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetCBVParam(0));
		rootParams.push_back(GetCBVParam(1));
		rootParams.push_back(GetCBVParam(2));

		// Create desc for root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
		rootSignDesc.Init(static_cast<UINT>(rootParams.size()), rootParams.data());
		rootSignDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize And Create RootSignature
		mRootSigns["standard"] = SerializeAndCreateRootSignature(md3dDevice, &rootSignDesc);
	}

	{
	/*
		uab<uint8> testUA;
	*/
		// Descriptor ranges
		std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		ranges.push_back(CD3DX12_DESCRIPTOR_RANGE(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 
			1, 0, 
			0, 0U
		));

		// Describe root parameters
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetTableParam(ranges));

		// Create desc for root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
		rootSignDesc.Init(static_cast<UINT>(rootParams.size()), rootParams.data());
		rootSignDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize And Create RootSignature
		mRootSigns["post"] = SerializeAndCreateRootSignature(md3dDevice, &rootSignDesc);
	}
}

void SceneGraphApp::BuildShaders()
{
	// Just load
	mShaders["vs"] = d3dUtil::LoadBinary(L"VertexShader.cso");
	mShaders["ps"] = d3dUtil::LoadBinary(L"PixelShader.cso");
	mShaders["postVS"] = d3dUtil::LoadBinary(L"postVertex.cso");
	mShaders["postPS"] = d3dUtil::LoadBinary(L"postPixel.cso");
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
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC postPsoDesc = opaquePsoDesc;
	postPsoDesc.InputLayout = { 
		mInputLayouts["post"].data(), 
		(UINT)mInputLayouts["post"].size() 
	};
	postPsoDesc.pRootSignature = mRootSigns["post"].Get();
	postPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["postVS"]->GetBufferPointer()),
		mShaders["postVS"]->GetBufferSize()
	};
	postPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["postPS"]->GetBufferPointer()),
		mShaders["postPS"]->GetBufferSize()
	};
	postPsoDesc.DepthStencilState.DepthEnable = false;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&postPsoDesc, IID_PPV_ARGS(&mPSOs["post"])));
}

void SceneGraphApp::BuildLights()
{
	mDirLights.push_back({
		{1.0f, 1.0f, 1.0f},
		{1.0f, 0.0f, 1.0f}
	});

	mPointLights.push_back({
		{0.8f, 0.2f, 0.0f},
		{0.0f, 1.0f, 0.0f}
	});

	mSpotLights.push_back({
		{0.1f, 0.7f, 0.2f},
		{1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f}
	});
}

void SceneGraphApp::BuildScene()
{
	// If we have a scene defination, we should load here.
	// Now we just hard-encode the scene.
	// So only initing mPassConstants is needed
	mPassConstants = std::make_unique<PassConstants>();
}

void SceneGraphApp::BuildTesting()
{
	// Build Resource
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = mClientWidth;
	desc.Height = mClientHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R32_UINT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mSumResource)
	));

	// Build Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 1;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&uavHeapDesc,
		IID_PPV_ARGS(&mUAVDescHeap)
	));
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&uavHeapDesc,
		IID_PPV_ARGS(&mUAVCPUDescHeap)
	));

	// Build Descriptor
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32_UINT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateUnorderedAccessView(
		mSumResource.Get(), nullptr,
		&uavDesc, mUAVDescHeap->GetCPUDescriptorHandleForHeapStart()
	);
	md3dDevice->CreateUnorderedAccessView(
		mSumResource.Get(), nullptr,
		&uavDesc, mUAVCPUDescHeap->GetCPUDescriptorHandleForHeapStart()
	);
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
	size_t totalLightNum = 0;
	totalLightNum += mDirLights.size();
	totalLightNum += mPointLights.size();
	totalLightNum += mSpotLights.size();
	if (totalLightNum > MAX_LIGHT_NUM)
		throw "Lights too many with " + std::to_string(totalLightNum);

	auto& content = mPassConstants->content;
	unsigned int li = 0;

	// direction lights
	for (const auto& dirLight : mDirLights) {
		LightConstants consts;

		// Reverse & Normalize direction for convenience
		XMVECTOR direction = XMLoadFloat3(&dirLight.Direction);
		direction = XMVectorScale(direction, -1.0f);
		direction = XMVector4Normalize(direction);
		XMStoreFloat4(&consts.content.Direction, direction);

		consts.content.Color = MathHelper::XMFLOAT3TO4(dirLight.Color, 1.0f);

		content.Lights[li] = consts.content;
		li++;
	}
	content.LightPerTypeNum.x = static_cast<UINT32>(mDirLights.size());

	// point lights
	for (const auto& pointLight : mPointLights) {
		LightConstants consts;

		consts.content.Pos = MathHelper::XMFLOAT3TO4(pointLight.Position, 1.0f);
		consts.content.Color = MathHelper::XMFLOAT3TO4(pointLight.Color, 1.0f);

		content.Lights[li] = consts.content;
		li++;
	}
	content.LightPerTypeNum.y = static_cast<UINT32>(mPointLights.size());

	// spot lights
	for (const auto& spotLight : mSpotLights) {
		LightConstants consts;

		// Reverse & Normalize direction for convenience
		XMVECTOR direction = XMLoadFloat3(&spotLight.Direction);
		direction = XMVectorScale(direction, -1.0f);
		direction = XMVector4Normalize(direction);
		XMStoreFloat4(&consts.content.Direction, direction);

		consts.content.Pos = MathHelper::XMFLOAT3TO4(spotLight.Position, 1.0f);
		consts.content.Color = MathHelper::XMFLOAT3TO4(spotLight.Color, 1.0f);

		content.Lights[li] = consts.content;
		li++;
	}
	content.LightPerTypeNum.z = static_cast<UINT32>(mSpotLights.size());
}

template <class T, class U>
void FillBufferInfoAndUpload(
		ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList,
		std::shared_ptr<MeshGeometry> geo, 
		std::vector<T> verts, std::vector<U> indices, 
		DXGI_FORMAT indiceFormat) {
	// Fill buffer info
	UINT vertByteSize = static_cast<UINT>(verts.size() * sizeof(T));
	UINT indexByteSize = static_cast<UINT>(indices.size() * sizeof(U));
	geo->VertexByteStride = sizeof(T);
	geo->VertexBufferByteSize = vertByteSize;
	geo->IndexFormat = indiceFormat;
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

void SceneGraphApp::BuildGeos()
{
	{
		struct Vertex {
			XMFLOAT3 pos;
		};

		// Create Geo
		auto geo = std::make_shared<MeshGeometry>();
		geo->Name = "background";

		// Generate Vertex Info
		std::vector<Vertex> verts = {
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
		SubmeshGeometry submesh;
		submesh.BaseVertexLocation = 0;
		submesh.StartIndexLocation = 0;
		submesh.IndexCount = static_cast<UINT>(indices.size());
		geo->DrawArgs["background"] = submesh;

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

	{
		struct Vertex {
			XMFLOAT3 pos;
			XMFLOAT3 normal;
		};

		// Create Geo
		auto geo = std::make_shared<MeshGeometry>();
		geo->Name = "triangle";

		// Generate Vertex Info
		GeometryGenerator geoGenerator;
		GeometryGenerator::MeshData boxMesh = geoGenerator.CreateBox(1.0, 1.0, 1.0, 1);
		std::vector<UINT32> indices = boxMesh.Indices32;
		std::vector<Vertex> verts;
		for (const auto& vert : boxMesh.Vertices) {
			verts.push_back({ vert.Position, vert.Normal });
		}

		// Create submesh
		SubmeshGeometry submesh;
		submesh.BaseVertexLocation = 0;
		submesh.StartIndexLocation = 0;
		submesh.IndexCount = static_cast<UINT>(indices.size());
		geo->DrawArgs["triangle"] = submesh;

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

void SceneGraphApp::BuildMaterials()
{
	auto whiteMtl = std::make_shared<MaterialConstants>();
	whiteMtl->content.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mMtlConsts["white"] = whiteMtl;
}

void SceneGraphApp::BuildAndUpdateMaterialConstantBuffers()
{
	// Build Buffers
	mMaterialConstantsBuffers = std::make_unique<UploadBuffer<MaterialConstants::Content>>(
		md3dDevice.Get(), 
		MaterialConstants::getTotalNum(), true
	);
	
	// Update Buffers
	for (auto mtl_pair : mMtlConsts) {
		auto mtl = mtl_pair.second;
		mMaterialConstantsBuffers->CopyData(
			mtl->getID(),
			mtl->content
		);
	}
}

void SceneGraphApp::BuildRenderItems()
{
	{
		// Create Object constants
		auto triConsts = std::make_shared<ObjectConstants>();
		triConsts->content.ModelMat = MathHelper::Identity4x4();
		mObjConsts["triangle"] = triConsts;

		// Create render items
		auto renderItem = std::make_shared<RenderItem>();
		renderItem->Geo = mGeos["triangle"];
		renderItem->Submesh = mGeos["triangle"]->DrawArgs["triangle"];
		renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		renderItem->MtlConsts = mMtlConsts["white"];
		renderItem->PSO = mPSOs["opaque"];
		renderItem->ObjConsts = mObjConsts["triangle"];

		// Save render items
		mRenderItemQueue.push_back(std::move(renderItem));
	}

	{
		// Create Object constants
		auto consts = std::make_shared<ObjectConstants>();
		consts->content.ModelMat = MathHelper::Identity4x4();
		mObjConsts["background"] = consts;

		// Create render items
		auto renderItem = std::make_shared<RenderItem>();
		renderItem->Geo = mGeos["background"];
		renderItem->Submesh = mGeos["background"]->DrawArgs["background"];
		renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		renderItem->MtlConsts = mMtlConsts["white"];
		renderItem->PSO = mPSOs["post"];
		renderItem->ObjConsts = mObjConsts["background"];

		// Save render items
		mBackgroundRenderItem = std::move(renderItem);
	}
}

void SceneGraphApp::BuildObjectConstantBuffers()
{
	mObjectConstantsBuffers = std::make_unique<UploadBuffer<ObjectConstants::Content>>(
		md3dDevice.Get(), 
		ObjectConstants::getTotalNum(), true
	);
}

void SceneGraphApp::OnResize()
{
	D3DApp::OnResize();

	ResizeMidRenderTarget();
}

void SceneGraphApp::Update(const GameTimer& gt)
{
	// Notice: Be careful, matrix need transpose.

	// Update Pass Constants
	{
		auto& content = mPassConstants->content;

		XMMATRIX viewMat = mCamera.GetViewMatrix();
		viewMat = XMMatrixTranspose(viewMat);
		XMStoreFloat4x4(&content.ViewMat, viewMat);


		XMMATRIX projMat = mCamera.GetOrthoProjMatrix(
			static_cast<float>(mClientWidth)/static_cast<float>(mClientHeight)
		);
		projMat = XMMatrixTranspose(projMat);
		XMStoreFloat4x4(&content.ProjMat, projMat);
	}

	// Update Object Constants
	{
		auto& content = mObjConsts["triangle"]->content;

		XMMATRIX modelMat = XMLoadFloat4x4(&MathHelper::Identity4x4());
		modelMat = XMMatrixTranspose(modelMat);
		XMStoreFloat4x4(&content.ModelMat, modelMat);
		
		XMFLOAT4X4 normalModelFloat4x4 = content.ModelMat;
		normalModelFloat4x4._14 = 0;
		normalModelFloat4x4._24 = 0;
		normalModelFloat4x4._34 = 0;
		normalModelFloat4x4._41 = 0;
		normalModelFloat4x4._42 = 0;
		normalModelFloat4x4._43 = 0;
		normalModelFloat4x4._44 = 1;
		XMMATRIX normalModelMat = XMLoadFloat4x4(&normalModelFloat4x4);
		normalModelMat = MathHelper::InverseTranspose(normalModelMat);
			// Has Transported above
		XMStoreFloat4x4(&content.NormalModelMat, normalModelMat);
	}

	// Update Pass Constant Buffers
	{
		mPassConstantsBuffers->CopyData(
			mPassConstants->getID(),
			mPassConstants->content
		);
	}

	// Update Object Constant Buffers
	{
		for (auto renderItem : mRenderItemQueue) {
			mObjectConstantsBuffers->CopyData(
				renderItem->ObjConsts->getID(),
				renderItem->ObjConsts->content
			);
		}
		mObjectConstantsBuffers->CopyData(
			mBackgroundRenderItem->ObjConsts->getID(),
			mBackgroundRenderItem->ObjConsts->content
		);
	}
}

void SceneGraphApp::Draw(const GameTimer& gt)
{
	// CommandList Start Recoding
	{
		// Reuse the memory associated with command recording.
		// We can only reset when the associated command lists have finished execution on the GPU.
		ThrowIfFailed(mDirectCmdListAlloc->Reset());

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	}

	// Set Viewports & ScissorRects
	{
		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);
	}

	// Set Descriptor Heaps
	{
		ID3D12DescriptorHeap* descHeaps[] = { mCBVSRVUAVHeap->GetHeap() };
		mCommandList->SetDescriptorHeaps(1, descHeaps);
	}

	// Draw Scene
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMidRenderTarget.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &mMidRTVCPUHandle, true, &DepthStencilView());

		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(mMidRTVCPUHandle, Colors::Black, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["standard"].Get());

		// Assign Pass Constants Buffer
		auto passCBGPUAddr = mPassConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 passCBElementByteSize = mPassConstantsBuffers->getElementByteSize();
		mCommandList->SetGraphicsRootConstantBufferView(
			2, passCBGPUAddr + mPassConstants->getID() * passCBElementByteSize
		);

		// Draw Render Items
		Microsoft::WRL::ComPtr<ID3D12PipelineState> nowPSO = nullptr;

		auto objCBGPUAddr = mObjectConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 objCBElementByteSize = mObjectConstantsBuffers->getElementByteSize();
		auto mtlCBGPUAddr = mMaterialConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 mtlCBElementByteSize = mMaterialConstantsBuffers->getElementByteSize();

		for (auto renderItem : mRenderItemQueue) {
			// Change PSO if needed
			if (!nowPSO || nowPSO.Get() != renderItem->PSO.Get()) {
				nowPSO = renderItem->PSO;
				mCommandList->SetPipelineState(nowPSO.Get());
			}

			// Set IA
			D3D12_VERTEX_BUFFER_VIEW VBVs[1] = {
				renderItem->Geo->VertexBufferView()
			};
			mCommandList->IASetVertexBuffers(0, 1, VBVs);
			mCommandList->IASetIndexBuffer(&renderItem->Geo->IndexBufferView());
			mCommandList->IASetPrimitiveTopology(renderItem->PrimitiveTopology);

			// Assign Material Constants Buffer
			mCommandList->SetGraphicsRootConstantBufferView(
				1, mtlCBGPUAddr + renderItem->MtlConsts->getID() * mtlCBElementByteSize
			);

			// Assign Object Constants Buffer
			mCommandList->SetGraphicsRootConstantBufferView(
				0, objCBGPUAddr + renderItem->ObjConsts->getID()*objCBElementByteSize
			);

			// Draw Call
			mCommandList->DrawIndexedInstanced(
				renderItem->Submesh.IndexCount, 
				1,
				renderItem->Submesh.StartIndexLocation,
				renderItem->Submesh.BaseVertexLocation,
				0
			);
		}
	}

	// Post Process
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMidRenderTarget.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["post"].Get());

		// Assign UAV
		mCommandList->SetGraphicsRootDescriptorTable(
			0, mMidSRVGPUHandle
		);

		// Draw Render Items
		auto& renderItem = mBackgroundRenderItem;
		mCommandList->SetPipelineState(renderItem->PSO.Get());

		// Set IA
		D3D12_VERTEX_BUFFER_VIEW VBVs[1] = {
			renderItem->Geo->VertexBufferView()
		};
		mCommandList->IASetVertexBuffers(0, 1, VBVs);
		mCommandList->IASetIndexBuffer(&renderItem->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(renderItem->PrimitiveTopology);

		// Draw Call
		mCommandList->DrawIndexedInstanced(
			renderItem->Submesh.IndexCount, 
			1,
			renderItem->Submesh.StartIndexLocation,
			renderItem->Submesh.BaseVertexLocation,
			0
		);
	}

	// CommandList End Recording
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		// Done recording commands.
		ThrowIfFailed(mCommandList->Close());
	}

	// Execute CommandList
	{
		// Add the command list to the queue for execution.
		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	}

	// Swap Chain
	{
		// swap the back and front buffers
		ThrowIfFailed(mSwapChain->Present(0, 0));
		mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	}

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

