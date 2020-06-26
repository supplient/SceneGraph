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
	// RTV
	{
		// Build Descriptor Heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 3;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		mRTVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		// Build Handle
		mOpaqueRTVCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
		mTransRTVCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
		mTransBlendRTVCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
	}

	// CBV, SRV, UAV
	// GPU heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 5;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;
		mCBVSRVUAVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		UINT index = mCBVSRVUAVHeap->Alloc();
		mOpaqueSRVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mOpaqueSRVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mTransSRVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mTransSRVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mTransBlendSRVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mTransBlendSRVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mZBufferSRVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mZBufferSRVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mNCountUAVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mNCountUAVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
	}

	// CBV, SRV, UAV
	// CPU heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;

		mCBVSRVUAVCPUHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		UINT index = mCBVSRVUAVCPUHeap->Alloc();
		mNCountUAVCPUHeapCPUHandle = mCBVSRVUAVCPUHeap->GetCPUHandle(index);
	}
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

		mInputLayouts["transBlend"] = { pos };
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

		uab<uint32> ncount;
		srb zbuffer;
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

		// Describe root parameters
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetCBVParam(0));
		rootParams.push_back(GetCBVParam(1));
		rootParams.push_back(GetCBVParam(2));
		rootParams.push_back(GetTableParam(nCountUAVranges));
		rootParams.push_back(GetTableParam(zbufferSRVranges));

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
}

void SceneGraphApp::BuildShaders()
{
	// Just load
	mShaders["vs"] = d3dUtil::LoadBinary(L"VertexShader.cso");
	mShaders["ps"] = d3dUtil::LoadBinary(L"PixelShader.cso");
	mShaders["transBlendVS"] = d3dUtil::LoadBinary(L"transBlendVertex.cso");
	mShaders["transBlendPS"] = d3dUtil::LoadBinary(L"transBlendPixel.cso");
	mShaders["transPS"] = d3dUtil::LoadBinary(L"transparentPixel.cso");
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
	opaquePsoDesc.RTVFormats[0] = mOpaqueRenderTargetFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

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
	transPsoDesc.RTVFormats[0] = mTransRenderTargetFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transPsoDesc, IID_PPV_ARGS(&mPSOs["trans"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC postPsoDesc = opaquePsoDesc;
	postPsoDesc.InputLayout = { 
		mInputLayouts["transBlend"].data(), 
		(UINT)mInputLayouts["transBlend"].size() 
	};
	postPsoDesc.pRootSignature = mRootSigns["transBlend"].Get();
	postPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["transBlendVS"]->GetBufferPointer()),
		mShaders["transBlendVS"]->GetBufferSize()
	};
	postPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["transBlendPS"]->GetBufferPointer()),
		mShaders["transBlendPS"]->GetBufferSize()
	};
	postPsoDesc.DepthStencilState.DepthEnable = false;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&postPsoDesc, IID_PPV_ARGS(&mPSOs["transBlend"])));
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
			XMFLOAT3 normal;
		};

		// Create Geo
		auto geo = std::make_shared<MeshGeometry>();
		geo->Name = "triangle";

		// Generate Vertex Info
		GeometryGenerator geoGenerator;
		std::vector<UINT32> indices;
		std::vector<Vertex> verts;
		{
			// Box
			GeometryGenerator::MeshData boxMesh = geoGenerator.CreateBox(1.0, 1.0, 1.0, 1);
			for (const auto& vert : boxMesh.Vertices) {
				verts.push_back({ vert.Position, vert.Normal });
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
			std::vector<UINT32> boardIndices = {
				0, 1, 2,
				1, 3, 2,
			};

			SubmeshGeometry submesh;
			submesh.BaseVertexLocation = static_cast<UINT>(verts.size());
			submesh.StartIndexLocation = static_cast<UINT>(indices.size());
			submesh.IndexCount = static_cast<UINT>(boardIndices.size());
			geo->DrawArgs["board"] = submesh;

			for (const auto& vert : boardVerts) {
				verts.push_back({ vert, {0.0f, 0.0f, -1.0f} });
			}
			indices.insert(indices.end(), boardIndices.begin(), boardIndices.end());
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
}

void SceneGraphApp::BuildMaterials()
{
	auto whiteMtl = std::make_shared<MaterialConstants>();
	whiteMtl->content.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mMtlConsts["white"] = whiteMtl;

	auto transBlueMtl = std::make_shared<MaterialConstants>();
	transBlueMtl->content.Diffuse = { 0.0f, 0.0f, 0.8f, 0.7f };
	mMtlConsts["transBlue"] = transBlueMtl;

	auto transRedMtl = std::make_shared<MaterialConstants>();
	transRedMtl->content.Diffuse = { 0.8f, 0.0f, 0.0f, 0.7f };
	mMtlConsts["transRed"] = transRedMtl;
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
	// Opaque
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
			renderItem->Geo = mGeos["triangle"];
			renderItem->Submesh = mGeos["triangle"]->DrawArgs["board"];
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->MtlConsts = mMtlConsts["transBlue"];
			renderItem->PSO = mPSOs["trans"];
			renderItem->ObjConsts = mObjConsts["transBlueBoard"];

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
			renderItem->Geo = mGeos["triangle"];
			renderItem->Submesh = mGeos["triangle"]->DrawArgs["board"];
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->MtlConsts = mMtlConsts["transRed"];
			renderItem->PSO = mPSOs["trans"];
			renderItem->ObjConsts = mObjConsts["transRedBoard"];

			// Save render items
			mTransRenderItemQueue.push_back(std::move(renderItem));
		}
	}

	// Post
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
		renderItem->PSO = mPSOs["transBlend"];
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

void SceneGraphApp::ResizeScreenUAVSRV()
{
	// NCount UAV
	{
		// Build Resources
		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = mClientWidth;
		desc.Height = mClientHeight;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = mNCountFormat;
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
			IID_PPV_ARGS(&mNCountResource)
		));

		// Build Descriptor
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = mNCountFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		md3dDevice->CreateUnorderedAccessView(
			mNCountResource.Get(), nullptr,
			&uavDesc, mNCountUAVCPUHandle
		);
		md3dDevice->CreateUnorderedAccessView(
			mNCountResource.Get(), nullptr,
			&uavDesc, mNCountUAVCPUHeapCPUHandle
		);
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
		desc.Format = mZBufferFormat;
		desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
		desc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(mZBufferResource.GetAddressOf())));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = mZBufferFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		md3dDevice->CreateShaderResourceView(
			mZBufferResource.Get(),
			&srvDesc, mZBufferSRVCPUHandle
		);
	}
}

void SceneGraphApp::ResizeOpaqueRenderTarget()
{
	// Build Resource
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = mClientWidth;
	desc.Height = mClientHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = mOpaqueRenderTargetFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = mOpaqueRenderTargetFormat;
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
		IID_PPV_ARGS(&mOpaqueRenderTarget)
	));

	// Build Descriptor
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mOpaqueRenderTargetFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateRenderTargetView(
		mOpaqueRenderTarget.Get(),
		&rtvDesc, mOpaqueRTVCPUHandle
	);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = mOpaqueRenderTargetFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	md3dDevice->CreateShaderResourceView(
		mOpaqueRenderTarget.Get(),
		&srvDesc, mOpaqueSRVCPUHandle
	);
}

void SceneGraphApp::ResizeTransRenderTarget()
{
	// Build Resource
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = mClientWidth;
	desc.Height = mClientHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = mTransRenderTargetFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = mTransRenderTargetFormat;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 0.0f;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&mTransRenderTarget)
	));

	// Build Descriptor
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mTransRenderTargetFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateRenderTargetView(
		mTransRenderTarget.Get(),
		&rtvDesc, mTransRTVCPUHandle
	);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = mTransRenderTargetFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	md3dDevice->CreateShaderResourceView(
		mTransRenderTarget.Get(),
		&srvDesc, mTransSRVCPUHandle
	);
}

void SceneGraphApp::ResizeTransBlendRenderTarget()
{
	// Build Resource
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = mClientWidth;
	desc.Height = mClientHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = mTransBlendRenderTargetFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = mTransBlendRenderTargetFormat;
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
		IID_PPV_ARGS(&mTransBlendRenderTarget)
	));

	// Build Descriptor
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mTransBlendRenderTargetFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateRenderTargetView(
		mTransBlendRenderTarget.Get(),
		&rtvDesc, mTransBlendRTVCPUHandle
	);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = mTransBlendRenderTargetFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	md3dDevice->CreateShaderResourceView(
		mTransBlendRenderTarget.Get(),
		&srvDesc, mTransBlendSRVCPUHandle
	);
}

void SceneGraphApp::DrawRenderItems(const std::vector<std::shared_ptr<RenderItem>>& renderItemQueue)
{
	Microsoft::WRL::ComPtr<ID3D12PipelineState> nowPSO = nullptr;

	auto objCBGPUAddr = mObjectConstantsBuffers->Resource()->GetGPUVirtualAddress();
	UINT64 objCBElementByteSize = mObjectConstantsBuffers->getElementByteSize();
	auto mtlCBGPUAddr = mMaterialConstantsBuffers->Resource()->GetGPUVirtualAddress();
	UINT64 mtlCBElementByteSize = mMaterialConstantsBuffers->getElementByteSize();

	for (auto renderItem : renderItemQueue) {
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

void SceneGraphApp::OnResize()
{
	D3DApp::OnResize();

	ResizeScreenUAVSRV();
	ResizeOpaqueRenderTarget();
	ResizeTransRenderTarget();
	ResizeTransBlendRenderTarget();
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
		for (auto renderItem : mTransRenderItemQueue) {
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

	// Refresh Frame Shared Data
	{
		UINT clearValues[4] = { 0, 0, 0, 0 };
		mCommandList->ClearUnorderedAccessViewUint(
			mNCountUAVGPUHandle, mNCountUAVCPUHeapCPUHandle,
			mNCountResource.Get(), clearValues,
			0, nullptr
		);
	}

	// Draw Scene
	{
		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["standard"].Get());

		// Assign Pass Constants Buffer
		auto passCBGPUAddr = mPassConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 passCBElementByteSize = mPassConstantsBuffers->getElementByteSize();
		mCommandList->SetGraphicsRootConstantBufferView(
			2, passCBGPUAddr + mPassConstants->getID() * passCBElementByteSize
		);

		// Assign UAV
		mCommandList->SetGraphicsRootDescriptorTable(
			3, mNCountUAVGPUHandle
		);

		// Assign ZBuffer for reference
		mCommandList->SetGraphicsRootDescriptorTable(
			4, mZBufferSRVGPUHandle
		);

		// Opaque
		{
			// Trans to Render Target
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOpaqueRenderTarget.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &mOpaqueRTVCPUHandle, true, &DepthStencilView());

			// Clear the back buffer and depth buffer.
			mCommandList->ClearRenderTargetView(mOpaqueRTVCPUHandle, Colors::Black, 0, nullptr);
			mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			// Draw Render Items
			DrawRenderItems(mRenderItemQueue);

			// Trans back to Shader Resource
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOpaqueRenderTarget.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}

		// Copy ZBuffer for reference
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mZBufferResource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
		mCommandList->CopyResource(mZBufferResource.Get(), mDepthStencilBuffer.Get());
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mZBufferResource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		// Transparent
		{
			// Trans to Render Target
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTransRenderTarget.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &mTransRTVCPUHandle, true, &DepthStencilView());

			// Clear Render Target
			FLOAT clearValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			mCommandList->ClearRenderTargetView(mTransRTVCPUHandle, clearValue, 0, nullptr);

			// Draw Render Items
			DrawRenderItems(mTransRenderItemQueue);

			// Trans back to Shader Resource
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTransRenderTarget.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}

	// Transparent Blend
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTransBlendRenderTarget.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &mTransBlendRTVCPUHandle, true, &DepthStencilView());

		// Clear the back buffer and depth buffer.
		// TODO this may needless
		FLOAT clearValue[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		mCommandList->ClearRenderTargetView(mTransBlendRTVCPUHandle, clearValue, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["transBlend"].Get());

		// Assign SRV
		mCommandList->SetGraphicsRootDescriptorTable(
			0, mOpaqueSRVGPUHandle
		);
		mCommandList->SetGraphicsRootDescriptorTable(
			1, mTransSRVGPUHandle
		);

		// Assign UAV
		mCommandList->SetGraphicsRootDescriptorTable(
			2, mNCountUAVGPUHandle
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

		// Trans back to Shader Resource
		// mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTransBlendRenderTarget.Get(),
			// D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	// Copy to BackBuffer
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTransBlendRenderTarget.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST));
	mCommandList->CopyResource(CurrentBackBuffer(), mTransBlendRenderTarget.Get());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTransBlendRenderTarget.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

	// CommandList End Recording
	{
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

