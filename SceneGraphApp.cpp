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

	// Init PSO concerned
	BuildInputLayout();
	BuildRootSignature();
	BuildShaders();
	BuildPSOs();

	// Init Scene
	BuildScene();
	
	// Init Scene Resources
	BuildPassConstantBuffers();
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
	cb perObject{
		float4x4 modelMat;
	}
	cb perPass{
		float4x4 viewMat;
		float4x4 projMat;
	}
*/
	// Describe root parameters
	CD3DX12_ROOT_PARAMETER rootParams[2];
	rootParams[0].InitAsConstantBufferView(0);
	rootParams[1].InitAsConstantBufferView(1);

	// Create desc for root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;
	rootSignDesc.Init(2, rootParams);
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
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
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

void SceneGraphApp::BuildGeos()
{
	// Create Geo
	auto geo = std::make_shared<MeshGeometry>();
	geo->Name = "triangle";

	// Generate Vertex Info
	GeometryGenerator geoGenerator;
	GeometryGenerator::MeshData boxMesh = geoGenerator.CreateBox(1.0, 1.0, 1.0, 1);
	std::vector<UINT32> indices = boxMesh.Indices32;
	std::vector<XMFLOAT3> verts;
	for (const auto& vert : boxMesh.Vertices) {
		verts.push_back(vert.Position);
	}
	UINT vertByteSize = static_cast<UINT>(verts.size() * sizeof(XMFLOAT3));
	UINT indexByteSize = static_cast<UINT>(indices.size() * sizeof(FLOAT));

	// Fill buffer info
	geo->VertexByteStride = sizeof(XMFLOAT3);
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

void SceneGraphApp::BuildRenderItems()
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
	renderItem->PSO = mPSOs["opaque"];
	renderItem->Consts = triConsts;

	// Save render items
	mRenderItemQueue.push_back(std::move(renderItem));
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
	/*
	{
		auto& content = mObjConsts["triangle"]->content;
		XMMATRIX modelMat = XMMatrixRotationZ(90.0f/180.0f*MathHelper::Pi);
		modelMat = XMMatrixTranspose(modelMat);
		XMStoreFloat4x4(&content.ModelMat, modelMat);
	}
	*/

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
				renderItem->Consts->getID(),
				renderItem->Consts->content
			);
		}
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
		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

		// Assign Pass Constants Buffer
		auto passCBGPUAddr = mPassConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 passCBElementByteSize = mPassConstantsBuffers->getElementByteSize();
		mCommandList->SetGraphicsRootConstantBufferView(
			1, passCBGPUAddr + mPassConstants->getID() * passCBElementByteSize
		);

		// Draw Render Items
		Microsoft::WRL::ComPtr<ID3D12PipelineState> nowPSO = nullptr;
		ID3D12RootSignature* nowRootSign = nullptr;
		auto objCBGPUAddr = mObjectConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 objCBElementByteSize = mObjectConstantsBuffers->getElementByteSize();

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

			// Assign Object Constants Buffer
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

