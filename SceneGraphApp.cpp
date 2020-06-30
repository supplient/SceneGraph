//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "SceneGraphApp.h"
#include "../Common/GeometryGenerator.h"
#include "Predefine.h"

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
	BuildRenderTargets();
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

	// Init Postprocess Resources
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

void SceneGraphApp::BuildRenderTargets()
{
	FLOAT clearValue[4];
	clearValue[0] = 0.0f;
	clearValue[1] = 0.0f;
	clearValue[2] = 0.0f;
	clearValue[3] = 0.0f;
	mRenderTargets["opaque"] = std::make_unique<RenderTarget>(
		L"opaque", DXGI_FORMAT_R8G8B8A8_UNORM, clearValue);
	mRenderTargets["trans"] = std::make_unique<RenderTarget>(
		L"transparent", DXGI_FORMAT_R32G32B32A32_FLOAT, clearValue);
	mRenderTargets["transBlend"] = std::make_unique<RenderTarget>(
		L"transparent blend", DXGI_FORMAT_R8G8B8A8_UNORM, clearValue);
	mRenderTargets["afterResolve"] = std::make_unique<RenderTarget>(
		L"afterResolve", DXGI_FORMAT_R8G8B8A8_UNORM, clearValue);
	mRenderTargets["fxaa"] = std::make_unique<RenderTarget>(
		L"fxaa", DXGI_FORMAT_R8G8B8A8_UNORM, clearValue,
		true, L"fxaa depth"
		);
}

void SceneGraphApp::BuildDescriptorHeaps()
{
	// DSV
	{
		// Build Descriptor Heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		mDSVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		// Build Handle
		mRenderTargets["fxaa"]->dsvCPUHandle = mDSVHeap->GetCPUHandle(mDSVHeap->Alloc());
	}

	// RTV
	{
		// Build Descriptor Heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 5;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		mRTVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		// Build Handle
		mRenderTargets["opaque"]->rtvCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
		mRenderTargets["trans"]->rtvCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
		mRenderTargets["transBlend"]->rtvCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
		mRenderTargets["afterResolve"]->rtvCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
		mRenderTargets["fxaa"]->rtvCPUHandle = mRTVHeap->GetCPUHandle(mRTVHeap->Alloc());
	}

	// CBV, SRV, UAV // GPU heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 7;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;
		mCBVSRVUAVHeap = std::make_unique<StaticDescriptorHeap>(
			md3dDevice, heapDesc
		);

		UINT index = mCBVSRVUAVHeap->Alloc();
		mRenderTargets["opaque"]->srvCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mRenderTargets["opaque"]->srvGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mRenderTargets["trans"]->srvCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mRenderTargets["trans"]->srvGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mRenderTargets["transBlend"]->srvCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mRenderTargets["transBlend"]->srvGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mRenderTargets["afterResolve"]->srvCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mRenderTargets["afterResolve"]->srvGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mRenderTargets["fxaa"]->srvCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mRenderTargets["fxaa"]->srvGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mZBufferSRVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mZBufferSRVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
		index = mCBVSRVUAVHeap->Alloc();
		mNCountUAVCPUHandle = mCBVSRVUAVHeap->GetCPUHandle(index);
		mNCountUAVGPUHandle = mCBVSRVUAVHeap->GetGPUHandle(index);
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

	{ // Standard
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

	{ // transBlend
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

	{ // FXAA
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

		// Describe root parameters
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.push_back(GetCBVParam(0));
		rootParams.push_back(GetTableParam(texSRVRanges));

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
	}
}

void SceneGraphApp::BuildShaders()
{
	static const std::string VS_TARGET = "vs_5_1";
	static const std::string PS_TARGET = "ps_5_1";

	// defines
	std::vector<D3D_SHADER_MACRO> defines;
	if(m4xMsaaState)
		defines.push_back({ "MULTIPLE_SAMPLE", "4" });
	std::string maxLightNumStr = std::to_string(MAX_LIGHT_NUM);
	defines.push_back({ "MAX_LIGHT_NUM", maxLightNumStr.c_str() });
	defines.push_back({ NULL, NULL });

	// compile
	mShaders["vs"] = d3dUtil::CompileShader(
		L"VertexShader.hlsl", defines.data(), "main", VS_TARGET
	);
	mShaders["ps"] = d3dUtil::CompileShader(
		L"PixelShader.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["transPS"] = d3dUtil::CompileShader(
		L"transPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["simpleVS"] = d3dUtil::CompileShader(
		L"simpleVertex.hlsl", defines.data(), "main", VS_TARGET
	);
	mShaders["transBlendPS"] = d3dUtil::CompileShader(
		L"transBlendPixel.hlsl", defines.data(), "main", PS_TARGET
	);
	mShaders["fxaaPS"] = d3dUtil::CompileShader(
		L"fxaaPixel.hlsl", defines.data(), "main", PS_TARGET
	);
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
	opaquePsoDesc.RTVFormats[0] = mRenderTargets["opaque"]->viewFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilViewFormat;
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
	transPsoDesc.RTVFormats[0] = mRenderTargets["trans"]->viewFormat;
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
	transBlendPsoDesc.RTVFormats[0] = mRenderTargets["transBlend"]->viewFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transBlendPsoDesc, IID_PPV_ARGS(&mPSOs["transBlend"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC fxaaPsoDesc = transBlendPsoDesc;
	fxaaPsoDesc.pRootSignature = mRootSigns["fxaa"].Get();
	fxaaPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["fxaaPS"]->GetBufferPointer()),
		mShaders["fxaaPS"]->GetBufferSize()
	};
	fxaaPsoDesc.RTVFormats[0] = mRenderTargets["fxaa"]->viewFormat;
	fxaaPsoDesc.SampleDesc.Count = 1;
	fxaaPsoDesc.SampleDesc.Quality = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&fxaaPsoDesc, IID_PPV_ARGS(&mPSOs["fxaa"])));
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
	/*
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
			XMMATRIX normalModelMat = MathHelper::GenNormalModelMat(consts->content.ModelMat);
				// Has Transported above
			XMStoreFloat4x4(&consts->content.NormalModelMat, normalModelMat);

			mObjConsts["blueBoard"] = consts;

			// Create render items
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->Geo = mGeos["triangle"];
			renderItem->Submesh = mGeos["triangle"]->DrawArgs["board"];
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->MtlConsts = mMtlConsts["blue"];
			renderItem->PSO = "opaque";
			renderItem->ObjConsts = mObjConsts["blueBoard"];

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
			XMMATRIX normalModelMat = MathHelper::GenNormalModelMat(consts->content.ModelMat);
				// Has Transported above
			XMStoreFloat4x4(&consts->content.NormalModelMat, normalModelMat);

			mObjConsts["redBoard"] = consts;

			// Create render items
			auto renderItem = std::make_shared<RenderItem>();
			renderItem->Geo = mGeos["triangle"];
			renderItem->Submesh = mGeos["triangle"]->DrawArgs["board"];
			renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			renderItem->MtlConsts = mMtlConsts["red"];
			renderItem->PSO = "opaque";
			renderItem->ObjConsts = mObjConsts["redBoard"];

			// Save render items
			mRenderItemQueue.push_back(std::move(renderItem));
		}
	}
	*/

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
		renderItem->PSO = "opaque";
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
			renderItem->PSO = "trans";
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
			renderItem->PSO = "trans";
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
	// NCount UAV
	{
		// Build Resources
		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = m4xMsaaState ? 4*mClientWidth : mClientWidth;
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
	opaqueDesc.Format = mRenderTargets["opaque"]->viewFormat;
	opaqueDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaqueDesc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality-1 : 0;
	opaqueDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	opaqueDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	mRenderTargets["opaque"]->Resize(
		md3dDevice,
		&opaqueDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// Transparent Rendering
	D3D12_RESOURCE_DESC transDesc = opaqueDesc;
	transDesc.Format = mRenderTargets["trans"]->viewFormat;

	mRenderTargets["trans"]->Resize(
		md3dDevice,
		&transDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// Transparent Blending
	D3D12_RESOURCE_DESC transBlendDesc = opaqueDesc;
	transBlendDesc.Format = mRenderTargets["transBlend"]->viewFormat;

	mRenderTargets["transBlend"]->Resize(
		md3dDevice,
		&transBlendDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// After Resolve
	D3D12_RESOURCE_DESC afterResolveDesc = transBlendDesc;
	afterResolveDesc.Format = mRenderTargets["afterResolve"]->viewFormat;
	afterResolveDesc.SampleDesc.Count = 1;
	afterResolveDesc.SampleDesc.Quality = 0;

	mRenderTargets["afterResolve"]->Resize(
		md3dDevice,
		&afterResolveDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// Fxaa
	D3D12_RESOURCE_DESC fxaaDesc = afterResolveDesc;
	fxaaDesc.Format = mRenderTargets["fxaa"]->viewFormat;

	D3D12_RESOURCE_DESC fxaaDSDesc;
	fxaaDSDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	fxaaDSDesc.Alignment = 0;
    fxaaDSDesc.Width = mClientWidth;
    fxaaDSDesc.Height = mClientHeight;
    fxaaDSDesc.DepthOrArraySize = 1;
    fxaaDSDesc.MipLevels = 1;
	fxaaDSDesc.Format = mRenderTargets["fxaa"]->dsResourceFormat;
	fxaaDSDesc.SampleDesc.Count = 1;
    fxaaDSDesc.SampleDesc.Quality = 0;
    fxaaDSDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    fxaaDSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	mRenderTargets["fxaa"]->Resize(
		md3dDevice,
		&fxaaDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&fxaaDSDesc
	);
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

void SceneGraphApp::DrawRenderItems(const std::vector<std::shared_ptr<RenderItem>>& renderItemQueue)
{
	Microsoft::WRL::ComPtr<ID3D12PipelineState> nowPSO = nullptr;

	auto objCBGPUAddr = mObjectConstantsBuffers->Resource()->GetGPUVirtualAddress();
	UINT64 objCBElementByteSize = mObjectConstantsBuffers->getElementByteSize();
	auto mtlCBGPUAddr = mMaterialConstantsBuffers->Resource()->GetGPUVirtualAddress();
	UINT64 mtlCBElementByteSize = mMaterialConstantsBuffers->getElementByteSize();

	for (auto renderItem : renderItemQueue) {
		// Change PSO if needed
		if (!nowPSO || nowPSO.Get() != mPSOs[renderItem->PSO].Get()) {
			nowPSO = mPSOs[renderItem->PSO];
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

void SceneGraphApp::OnMsaaStateChange()
{
	D3DApp::OnMsaaStateChange();

	BuildShaders();
	BuildPSOs();
}

void SceneGraphApp::OnResize()
{
	D3DApp::OnResize();

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	ResizeScreenUAVSRV();
	ResizeRenderTargets();
	ResizeFxaa();

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();
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

	// Update Fxaa Constant Buffers
	{
		mFxaaConstantsBuffers->CopyData(
			mFxaaConstants->getID(),
			mFxaaConstants->content
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
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["opaque"]->resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &mRenderTargets["opaque"]->rtvCPUHandle, true, &DepthStencilView());

			// Clear the back buffer and depth buffer.
			mCommandList->ClearRenderTargetView(mRenderTargets["opaque"]->rtvCPUHandle, mRenderTargets["opaque"]->clearValue, 0, nullptr);
			mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			// Draw Render Items
			DrawRenderItems(mRenderItemQueue);

			// Trans back to Shader Resource
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["opaque"]->resource.Get(),
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
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["trans"]->resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

			// Specify the buffers we are going to render to.
			mCommandList->OMSetRenderTargets(1, &mRenderTargets["trans"]->rtvCPUHandle, true, &DepthStencilView());

			// Clear Render Target
			mCommandList->ClearRenderTargetView(mRenderTargets["trans"]->rtvCPUHandle, mRenderTargets["trans"]->clearValue, 0, nullptr);

			// Draw Render Items
			DrawRenderItems(mTransRenderItemQueue);

			// Trans back to Shader Resource
			mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["trans"]->resource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}

	// Transparent Blend
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["transBlend"]->resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &mRenderTargets["transBlend"]->rtvCPUHandle, true, &DepthStencilView());

		// Clear the back buffer and depth buffer.
		// TODO this may needless
		mCommandList->ClearRenderTargetView(mRenderTargets["transBlend"]->rtvCPUHandle, mRenderTargets["transBlend"]->clearValue, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["transBlend"].Get());

		// Assign SRV
		mCommandList->SetGraphicsRootDescriptorTable(
			0, mRenderTargets["opaque"]->srvGPUHandle
		);
		mCommandList->SetGraphicsRootDescriptorTable(
			1, mRenderTargets["trans"]->srvGPUHandle
		);

		// Assign UAV
		mCommandList->SetGraphicsRootDescriptorTable(
			2, mNCountUAVGPUHandle
		);

		// Draw Render Items
		auto& renderItem = mBackgroundRenderItem;
		mCommandList->SetPipelineState(mPSOs["transBlend"].Get());

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

	// Resolve/Just copy
	// TODO This copy is needless
	if (m4xMsaaState) {
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["transBlend"]->resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["afterResolve"]->resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST));
		mCommandList->ResolveSubresource(
			mRenderTargets["afterResolve"]->resource.Get(), 0, 
			mRenderTargets["transBlend"]->resource.Get(), 0, 
			mBackBufferResourceFormat
		);
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["transBlend"]->resource.Get(),
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["afterResolve"]->resource.Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
	else {
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["transBlend"]->resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["afterResolve"]->resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
		mCommandList->CopyResource(
			mRenderTargets["afterResolve"]->resource.Get(), 
			mRenderTargets["transBlend"]->resource.Get()
		);
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["transBlend"]->resource.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["afterResolve"]->resource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	// FXAA
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["fxaa"]->resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &mRenderTargets["fxaa"]->rtvCPUHandle, true, &mRenderTargets["fxaa"]->dsvCPUHandle);

		// Clear the back buffer and depth buffer.
		// TODO this may needless
		mCommandList->ClearRenderTargetView(mRenderTargets["fxaa"]->rtvCPUHandle, mRenderTargets["fxaa"]->clearValue, 0, nullptr);
		mCommandList->ClearDepthStencilView(
			mRenderTargets["fxaa"]->dsvCPUHandle, 
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
			mRenderTargets["fxaa"]->depthClearValue, mRenderTargets["fxaa"]->stencilClearValue, 
			0, nullptr
		);

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["fxaa"].Get());

		// Assign CBV
		auto fxaaCBGPUAddr = mFxaaConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 fxaaCBElementByteSize = mFxaaConstantsBuffers->getElementByteSize();
		mCommandList->SetGraphicsRootConstantBufferView(
			0, fxaaCBGPUAddr + mFxaaConstants->getID() * fxaaCBElementByteSize
		);

		// Assign SRV
		mCommandList->SetGraphicsRootDescriptorTable(
			1, mRenderTargets["afterResolve"]->srvGPUHandle
		);

		// Draw Render Items
		auto& renderItem = mBackgroundRenderItem;
		mCommandList->SetPipelineState(mPSOs["fxaa"].Get());

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

		// Trans back states
		// mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["fxaa"]->resource.Get(),
			// D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	// Copy to BackBuffer
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["fxaa"]->resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST));
		mCommandList->CopyResource(
			CurrentBackBuffer(),
			mRenderTargets["fxaa"]->resource.Get()
		);
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets["fxaa"]->resource.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));
	}

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

std::vector<CD3DX12_STATIC_SAMPLER_DESC> SceneGraphApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC bilinearWrap(
		0,
		D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	return {
		bilinearWrap
	};
}

