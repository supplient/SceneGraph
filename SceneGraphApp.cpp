//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "SceneGraphApp.h"

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

	// Init PSO concerned
	BuildInputLayout();
	BuildRootSignature();
	BuildShaders();
	BuildPSOs();

	// If we have a scene defination, we should load here.
	// Now we just hard-encode the scene.
	
	// Init Scene Resources
	BuildGeos();

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

	return true;
}

void SceneGraphApp::BuildInputLayout()
{
/*
	Vertex {
		float3 pos;
	}
*/
	// Describe vertex input element
	D3D12_INPUT_ELEMENT_DESC pos;
	pos.SemanticName = "POS";
	pos.SemanticIndex = 0;
	pos.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	pos.AlignedByteOffset = 0;
	pos.InputSlot = 0;
	pos.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	pos.InstanceDataStepRate = 0;

	mInputLayout = { pos };
}

void SceneGraphApp::BuildRootSignature()
{
/*
	cb {
		float4x4 mvpMat;
	}
*/
	// Describe root parameters
	CD3DX12_ROOT_PARAMETER rootParams[1];
	rootParams[0].InitAsConstantBufferView(0);

	// Create desc for root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
	rootSignDesc.Init(1, rootParams);
	rootSignDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Serialize
	ComPtr<ID3DBlob> serializedRootSignBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	auto hr = D3D12SerializeRootSignature(
		&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignBlob.GetAddressOf(), errorBlob.GetAddressOf()
	);

	// Check if failed
	if (errorBlob != nullptr)
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	// Create root signature
	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0, 
		serializedRootSignBlob->GetBufferPointer(),
		serializedRootSignBlob->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)
	));
}

void SceneGraphApp::BuildShaders()
{
	// Just load
	mShaders["vs"] = d3dUtil::LoadBinary(L"VertexShader.cso");
	mShaders["ps"] = d3dUtil::LoadBinary(L"PixelShader.cso");
}

void SceneGraphApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
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
	mPSODescs["opaque"] = opaquePsoDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
}

void SceneGraphApp::BuildGeos()
{
	// Create Geo
	auto geo = std::make_shared<MeshGeometry>();
	geo->Name = "triangle";

	// Generate Vertex Info
	std::vector<FLOAT> verts = {
		1.0, 1.0, 0.5,
		0, 0, 0.5,
		0, 1.0, 0.5,
	};
	std::vector<UINT32> indices = {
		0, 1, 2
	};
	UINT vertByteSize = static_cast<UINT>(verts.size() * sizeof(FLOAT));
	UINT indexByteSize = static_cast<UINT>(indices.size() * sizeof(FLOAT));

	// Fill buffer info
	geo->VertexByteStride = sizeof(FLOAT) * 3;
	geo->VertexBufferByteSize = vertByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = indexByteSize;

	// Upload to GPU
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(),
		(const void*)verts.data(), vertByteSize,
		geo->VertexBufferUploader
	);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(),
		(const void*)indices.data(), indexByteSize,
		geo->IndexBufferUploader
	);

	// Create submesh
	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;
	submesh.IndexCount = static_cast<UINT>(indices.size());

	// Fill submeshes into geo
	geo->DrawArgs["triangle"] = submesh;

	// Save geo
	mGeos[geo->Name] = std::move(geo);
}

void SceneGraphApp::BuildObjectConstantBuffers()
{
	mObjectConstantBuffers = std::make_unique<UploadBuffer<ObjectConstants::Content>>(
		md3dDevice.Get(), 
		ObjectConstants::getTotalNum(), true
	);
}

void SceneGraphApp::BuildRenderItems()
{
	// Create render items
	auto renderItem = std::make_shared<RenderItem>();
	renderItem->Geo = mGeos["triangle"];
	renderItem->Submesh = mGeos["triangle"]->DrawArgs["triangle"];
	renderItem->PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	renderItem->PSO = mPSOs["opaque"];
	renderItem->RootSignature = mPSODescs["opaque"].pRootSignature;

	auto consts = std::make_shared<ObjectConstants>();
	consts->content.MVPMat = MathHelper::Identity4x4();
	renderItem->Consts = std::move(consts);

	// Save render items
	mRenderItems.push_back(std::move(renderItem));
}

void SceneGraphApp::OnResize()
{
	D3DApp::OnResize();
}

void SceneGraphApp::Update(const GameTimer& gt)
{
	// Update Object Constant Buffers
	for (auto renderItem : mRenderItems) {
		mObjectConstantBuffers->CopyData(
			renderItem->Consts->getID(),
			renderItem->Consts->content
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

	// Set Render Target
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	}

	// Set Viewport & Background
	{
		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}

	// Draw Scene
	{

		// Draw Render Items
		Microsoft::WRL::ComPtr<ID3D12PipelineState> nowPSO = nullptr;
		ID3D12RootSignature* nowRootSign = nullptr;
		auto objCBGPUAddr = mObjectConstantBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 objCBElementByteSize = mObjectConstantBuffers->getElementByteSize();

		for (auto renderItem : mRenderItems) {
			// Change PSO & Root Signature if needed
			if (!nowRootSign || nowRootSign != renderItem->RootSignature) {
				nowRootSign = renderItem->RootSignature;
				mCommandList->SetGraphicsRootSignature(nowRootSign);
			}
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

			// Assign root parameters
			mCommandList->SetGraphicsRootConstantBufferView(
				0, objCBGPUAddr + renderItem->Consts->getID()*objCBElementByteSize
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