#include "SceneGraphApp.h"
#include "PIXHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void TransResourceState(
	ComPtr<ID3D12GraphicsCommandList> commandList,
	std::vector<ID3D12Resource*> resources,
	std::vector<D3D12_RESOURCE_STATES> fromStates,
	std::vector<D3D12_RESOURCE_STATES> toStates
) {
	if (resources.size() != fromStates.size() || fromStates.size() != toStates.size())
		throw "Size does not match.";
	if (!resources.size())
		return;
	std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
	for (UINT i = 0; i < resources.size(); i++) {
		barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
			resources[i],
			fromStates[i], toStates[i]
		));
	}
	commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
}

void TransRT2SR(
	ComPtr<ID3D12GraphicsCommandList> commandList,
	std::vector<ID3D12Resource*> resources
) {
	std::vector<D3D12_RESOURCE_STATES> renderTargetStates(resources.size(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	std::vector<D3D12_RESOURCE_STATES> pixelResourceStates(resources.size(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	TransResourceState(
		commandList,
		resources,
		renderTargetStates,
		pixelResourceStates
	);
}

void TransSR2RT(
	ComPtr<ID3D12GraphicsCommandList> commandList,
	std::vector<ID3D12Resource*> resources
) {
	std::vector<D3D12_RESOURCE_STATES> renderTargetStates(resources.size(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	std::vector<D3D12_RESOURCE_STATES> pixelResourceStates(resources.size(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	TransResourceState(
		commandList,
		resources,
		pixelResourceStates,
		renderTargetStates
	);
}


void CopyResource(
	ComPtr<ID3D12GraphicsCommandList> commandList,
	ID3D12Resource* dst,
	D3D12_RESOURCE_STATES dstFromState, D3D12_RESOURCE_STATES dstToState,
	ID3D12Resource* src,
	D3D12_RESOURCE_STATES srcFromState, D3D12_RESOURCE_STATES srcToState
) {
	TransResourceState(
		commandList,
		{ src, dst },
		{ srcFromState, dstFromState },
		{ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST }
	);

	commandList->CopyResource(dst, src);

	TransResourceState(
		commandList,
		{ src, dst },
		{ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST },
		{ srcToState, dstToState }
	);
};


struct ShadowPassRenderParams
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	UINT RTVSize;

	UINT passCBRootParamIndex = -1;
	D3D12_GPU_VIRTUAL_ADDRESS passCBBaseAddr;
	UINT64 passCBByteSize;

	UINT objCBRootParamIndex = -1;
	D3D12_GPU_VIRTUAL_ADDRESS objCBBaseAddr;
	UINT64 objCBByteSize;

	UINT mtlCBRootParamIndex = -1;
	D3D12_GPU_VIRTUAL_ADDRESS mtlCBBaseAddr;
	UINT64 mtlCBByteSize;
};

void DrawRenderItems(
	const ShadowPassRenderParams& rps,
	const std::vector<std::shared_ptr<RenderItem>>& renderQueue
)
{
	for(auto renderItem: renderQueue)
	{
		// Set IA
		Mesh* mesh = Mesh::FindObjectByID(renderItem->MeshID);
		SubMesh submesh = mesh->GetSubMesh(renderItem->SubMeshID);
		D3D12_VERTEX_BUFFER_VIEW VBVs[1] = {
			mesh->GetVertexBufferView()
		};
		rps.commandList->IASetVertexBuffers(0, 1, VBVs);
		rps.commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
		rps.commandList->IASetPrimitiveTopology(submesh.primitiveTopology);

		// Assign Material Constants Buffer
		if (rps.mtlCBRootParamIndex != -1) {
			UINT mtlID = renderItem->MaterialID;
			rps.commandList->SetGraphicsRootConstantBufferView(
				rps.mtlCBRootParamIndex,
				rps.mtlCBBaseAddr + mtlID * rps.mtlCBByteSize
			);
		}

		// Assign Object Constants Buffer
		if (rps.objCBRootParamIndex != -1) {
			rps.commandList->SetGraphicsRootConstantBufferView(
				rps.objCBRootParamIndex, 
				rps.objCBBaseAddr + renderItem->ObjectID * rps.objCBByteSize
			);
		}

		// Draw Call
		rps.commandList->DrawIndexedInstanced(
			submesh.indexCount, 
			1,
			submesh.startIndexLoc,
			submesh.baseVertexLoc,
			0
		);
	}
}

void DrawPass(
	const ShadowPassRenderParams& rps,

	UINT passCBID,
	RenderTarget* colorRenderTarget,
	RenderTarget* depthRenderTarget,
	const std::vector<std::shared_ptr<RenderItem>>& renderQueue,

	bool clearColorRT=true, 
	bool clearDepthRT=true,
	UINT RTVOffset=0
)
{
	// Assign Pass Constants
	if (passCBID != -1) {
		rps.commandList->SetGraphicsRootConstantBufferView(
			rps.passCBRootParamIndex, 
			rps.passCBBaseAddr + passCBID * rps.passCBByteSize
		);
	}

	if (colorRenderTarget && depthRenderTarget) {
		// Set RenderTarget
		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(colorRenderTarget->GetRTVCPUHandle());
		RTVHandle.Offset(RTVOffset, rps.RTVSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE DSVHandle(depthRenderTarget->GetDSVCPUHandle());

		rps.commandList->OMSetRenderTargets(
			1, &RTVHandle,
			true, &DSVHandle
		);

		// Clear RenderTarget
		if(clearColorRT)
			rps.commandList->ClearRenderTargetView(
				RTVHandle,
				colorRenderTarget->GetColorClearValue(),
				0, nullptr
			);
		if(clearDepthRT)
			rps.commandList->ClearDepthStencilView(
				DSVHandle,
				D3D12_CLEAR_FLAG_DEPTH,
				depthRenderTarget->GetDepthClearValue(),
				depthRenderTarget->GetStencilClearValue(),
				0, nullptr
			);
	}

	// Draw Render Items
	DrawRenderItems(rps, renderQueue);
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

	// Set Descriptor Heaps
	{
		ID3D12DescriptorHeap* descHeaps[] = { mCBVSRVUAVHeap->GetHeap() };
		mCommandList->SetDescriptorHeaps(1, descHeaps);
	}

	// Refresh Frame Shared Data
	{
		mCommandList->ClearUnorderedAccessViewUint(
			mUABs["nCount"]->GetGPUHandle(), 
			mUABs["nCount"]->GetCPUHandle_CPUHeap(),
			mUABs["nCount"]->GetResource(),
			(UINT*)mUABs["nCount"]->GetClearValue(),
			0, nullptr
		);
	}

	// Set Viewports & ScissorRects
	{
		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		mCommandList->RSSetViewports(1, &mShadowScreenViewport);
		mCommandList->RSSetScissorRects(1, &mShadowScissorRect);
	}

	// Draw Shadows
	{
		PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Draw Shadows");

		auto& signPI = mRootSignParamIndices["shadow"];

		// Build Render Params
		ShadowPassRenderParams rps;
		rps.commandList = mCommandList;
		rps.RTVSize = mCbvSrvUavDescriptorSize;

		rps.passCBRootParamIndex = signPI["passCB"];
		rps.passCBBaseAddr = mShadowPassConstantsBuffers->Resource()->GetGPUVirtualAddress();
		rps.passCBByteSize = mShadowPassConstantsBuffers->getElementByteSize();

		rps.objCBRootParamIndex = signPI["objectCB"];
		rps.objCBBaseAddr = mObjectConstantsBuffers->Resource()->GetGPUVirtualAddress();
		rps.objCBByteSize = mObjectConstantsBuffers->getElementByteSize();

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["shadow"].Get());

		// Set PSOs
		mCommandList->SetPipelineState(mPSOs["shadow"].Get());


		// Direction Lights
		{
			PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Dir Shadows");
			for (int i = 0; i < mDirLights.size(); i++) {
				auto& dirLight = mDirLights[i];
				PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Dir Shadow %i", i);
				DrawPass(
					rps,
					dirLight.PassConstants->getID(),
					dirLight.ShadowRT.get(),
					dirLight.ShadowRT.get(),
					mOpaqueRenderItemQueue
				);
			}
		}

		// Spot Lights
		{
			PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Spot Shadows");
			for (int i = 0; i < mSpotLights.size(); i++) {
				auto& spotLight = mSpotLights[i];
				PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Spot Light %i", i);
				DrawPass(
					rps,
					spotLight.PassConstants->getID(),
					spotLight.ShadowRT.get(),
					spotLight.ShadowRT.get(),
					mOpaqueRenderItemQueue
				);
			}
		}

		// Point Lights
		{
			PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Point Shadows");
			// Set PSOs
			mCommandList->SetPipelineState(mPSOs["pointShadow"].Get());

			// Draw
			int li = 0;
			for (auto& pointLight : mPointLights) {
				PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Point Shadow %i", li);
				CD3DX12_CPU_DESCRIPTOR_HANDLE RTVCPUHandle(pointLight.ShadowRT->GetRTVCPUHandle());
				auto DSVCPUHandle = pointLight.ShadowRT->GetDSVCPUHandle();

				for (int i = 0; i < PointLight::RTVNum; i++) {
					PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Point Shadow Face %i", i);

					DrawPass(
						rps,
						pointLight.PassConstantsArray[i]->getID(),
						pointLight.ShadowRT.get(),
						pointLight.ShadowRT.get(),
						mOpaqueRenderItemQueue,
						true, true,
						i
					);
				}
				li++;
			}
		}
	}

	// Set Viewports & ScissorRects
	{
		// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);
	}

	// Vars for convenience. Note, do not alloc or free for these vars, only referece.
	RenderTarget* nowColorRenderTarget = nullptr;
	RenderTarget* nowDSRenderTarget = nullptr;

	// Switch shadow resources' states to pixel shader resource
	std::vector<ID3D12Resource*> shadowResources;
	for (const auto& light : mDirLights)
		shadowResources.push_back(light.ShadowRT->GetColorResource());
	for (const auto& light : mPointLights)
		shadowResources.push_back(light.ShadowRT->GetColorResource());
	for (const auto& light : mSpotLights)
		shadowResources.push_back(light.ShadowRT->GetColorResource());
	TransRT2SR(mCommandList, shadowResources);

	// Draw Scene
	{
		PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Draw Scenes");

		auto& signPI = mRootSignParamIndices["standard"]; // root sign param indices

		// Build Render Params
		ShadowPassRenderParams rps;
		rps.commandList = mCommandList;
		rps.RTVSize = mCbvSrvUavDescriptorSize;

		rps.passCBRootParamIndex = signPI["passCB"];
		rps.passCBBaseAddr = mPassConstantsBuffers->Resource()->GetGPUVirtualAddress();
		rps.passCBByteSize = mPassConstantsBuffers->getElementByteSize();

		rps.objCBRootParamIndex = signPI["objectCB"];
		rps.objCBBaseAddr = mObjectConstantsBuffers->Resource()->GetGPUVirtualAddress();
		rps.objCBByteSize = mObjectConstantsBuffers->getElementByteSize();

		rps.mtlCBRootParamIndex = signPI["materialCB"];
		rps.mtlCBBaseAddr = mMaterialConstantsBuffers->Resource()->GetGPUVirtualAddress();
		rps.mtlCBByteSize = mMaterialConstantsBuffers->getElementByteSize();

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["standard"].Get());

		// Assign Textures
		{
			PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Assign Textures");

			// Assign File Textures
			if (signPI.find("texSR") != signPI.end()) {
				mCommandList->SetGraphicsRootDescriptorTable(
					signPI["texSR"], mTexGPUHandleStart
				);
			}

			// Assign Light Shadow Textures
			if (signPI.find("spotShadowSR") != signPI.end()) {
				mCommandList->SetGraphicsRootDescriptorTable(
					signPI["spotShadowSR"], mSpotShadowTexGPUHandleStart
				);
			}
			if (signPI.find("dirShadowSR") != signPI.end()) {
				mCommandList->SetGraphicsRootDescriptorTable(
					signPI["dirShadowSR"], mDirShadowTexGPUHandleStart
				);
			}
			if (signPI.find("pointShadowSR") != signPI.end()) {
				mCommandList->SetGraphicsRootDescriptorTable(
					signPI["pointShadowSR"], mPointShadowTexGPUHandleStart
				);
			}
		}

		// Assign UAV
		mCommandList->SetGraphicsRootDescriptorTable(
			signPI["ncountUA"], mUABs["nCount"]->GetGPUHandle()
		);

		// Assign ZBuffer for reference
		mCommandList->SetGraphicsRootDescriptorTable(
			signPI["zbufferSR"], mZBufferSRVGPUHandle
		);

		// Set PSO
		mCommandList->SetPipelineState(mPSOs["opaque"].Get());

		// Opaque
		nowColorRenderTarget = mRenderTargets["opaque"].get();
		nowDSRenderTarget = mRenderTargets["opaque"].get();
		{
			PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Draw Opaque");
			DrawPass(
				rps,
				mPassConstants->getID(),
				mRenderTargets["opaque"].get(),
				mRenderTargets["opaque"].get(),
				mOpaqueRenderItemQueue
			);
		}

		// Copy ZBuffer for reference
		CopyResource(mCommandList,
			mZBufferResource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			mRenderTargets["opaque"]->GetDepthStencilResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_WRITE
		);

		/* Transparent does not work now

		// Set PSO
		mCommandList->SetPipelineState(mPSOs["trans"].Get());

		// Transparent
		nowColorRenderTarget = mRenderTargets["trans"].get();
		{
			PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Draw transparent");
			DrawPass(
				rps,
				-1,
				nowColorRenderTarget,
				nowDSRenderTarget,
				mTransRenderItemQueue,
				true, false
			);
		}
		*/
	}

	// Switch shadow resources' states back to render targets
	TransSR2RT(mCommandList, shadowResources);

	/* HBAO not work now
	// HBAO
	bool hasHBAO = false; // DEBUG
	nowColorRenderTarget = mRenderTargets["hbao"].get();
	if(hasHBAO)
	{
		auto& signPI = mRootSignParamIndices["hbao"]; // root sign param indices

		// Trans Previous RenderTarget to Shader Resource
		TransResourceState(
			mCommandList,
			{ mRenderTargets["opaque"]->GetColorResource() },
			{ D3D12_RESOURCE_STATE_RENDER_TARGET },	
			{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE }
		);

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(
			1, &nowColorRenderTarget->GetRTVCPUHandle(), 
			true, &nowDSRenderTarget->GetDSVCPUHandle()
		);

		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(
			nowColorRenderTarget->GetRTVCPUHandle(), 
			nowColorRenderTarget->GetColorClearValue(), 
			0, nullptr
		);
		mCommandList->ClearDepthStencilView(
			nowDSRenderTarget->GetDSVCPUHandle(), 
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
			nowDSRenderTarget->GetDepthClearValue(),  
			nowDSRenderTarget->GetStencilClearValue(),
			0, nullptr
		);

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["hbao"].Get());

		// Assign CBV
		auto hbaoCBGPUAddr = mHbaoConstantsBuffers->Resource()->GetGPUVirtualAddress();
		UINT64 hbaoCBElementByteSize = mHbaoConstantsBuffers->getElementByteSize();
		mCommandList->SetGraphicsRootConstantBufferView(
			signPI["hbaoCB"], hbaoCBGPUAddr + mHbaoConstants->getID() * hbaoCBElementByteSize
		);

		// Assign SRV
		mCommandList->SetGraphicsRootDescriptorTable(
			signPI["depthSRV"], mZBufferSRVGPUHandle
		);
		mCommandList->SetGraphicsRootDescriptorTable(
			signPI["colorSRV"], mRenderTargets["opaque"]->GetSRVGPUHandle()
		);

		// Draw Render Items
		auto& renderItem = mBackgroundRenderItem;
		mCommandList->SetPipelineState(mPSOs["hbao"].Get());

		// Set IA
		Mesh* mesh = Mesh::FindObjectByID(renderItem->MeshID);
		SubMesh submesh = mesh->GetSubMesh(renderItem->SubMeshID);
		D3D12_VERTEX_BUFFER_VIEW VBVs[1] = {
			mesh->GetVertexBufferView()
		};
		mCommandList->IASetVertexBuffers(0, 1, VBVs);
		mCommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
		mCommandList->IASetPrimitiveTopology(submesh.primitiveTopology);

		// Draw Call
		mCommandList->DrawIndexedInstanced(
			submesh.indexCount, 
			1,
			submesh.startIndexLoc,
			submesh.baseVertexLoc,
			0
		);

		// Turn Back
		TransResourceState(
			mCommandList,
			{ mRenderTargets["opaque"]->GetColorResource() },
			{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
			{ D3D12_RESOURCE_STATE_RENDER_TARGET}
		);
	}
	*/

	/* Transparent does not work now
	// Transparent Blend
	nowColorRenderTarget = mRenderTargets["transBlend"].get();
	{
		// Trans Previous RenderTarget to Shader Resource
		auto& prevRenderTarget = hasHBAO ? mRenderTargets["hbao"] : mRenderTargets["opaque"];
		TransResourceState(
			mCommandList,
			{ prevRenderTarget->GetColorResource(), mRenderTargets["trans"]->GetColorResource() },
			{ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET },
			{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE }
		);

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(
			1, &nowColorRenderTarget->GetRTVCPUHandle(), 
			true, &nowDSRenderTarget->GetDSVCPUHandle()
		);

		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(
			nowColorRenderTarget->GetRTVCPUHandle(), 
			nowColorRenderTarget->GetColorClearValue(), 
			0, nullptr
		);
		mCommandList->ClearDepthStencilView(
			nowDSRenderTarget->GetDSVCPUHandle(), 
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
			nowDSRenderTarget->GetDepthClearValue(),  
			nowDSRenderTarget->GetStencilClearValue(),
			0, nullptr
		);

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["transBlend"].Get());

		// Assign SRV
		mCommandList->SetGraphicsRootDescriptorTable(
			0, prevRenderTarget->GetSRVGPUHandle()
		);
		mCommandList->SetGraphicsRootDescriptorTable(
			1, mRenderTargets["trans"]->GetSRVGPUHandle()
		);

		// Assign UAV
		mCommandList->SetGraphicsRootDescriptorTable(
			2, mUABs["nCount"]->GetGPUHandle()
		);

		// Draw Render Items
		auto& renderItem = mBackgroundRenderItem;
		mCommandList->SetPipelineState(mPSOs["transBlend"].Get());

		// Set IA
		Mesh* mesh = Mesh::FindObjectByID(renderItem->MeshID);
		SubMesh submesh = mesh->GetSubMesh(renderItem->SubMeshID);
		D3D12_VERTEX_BUFFER_VIEW VBVs[1] = {
			mesh->GetVertexBufferView()
		};
		mCommandList->IASetVertexBuffers(0, 1, VBVs);
		mCommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
		mCommandList->IASetPrimitiveTopology(submesh.primitiveTopology);

		// Draw Call
		mCommandList->DrawIndexedInstanced(
			submesh.indexCount, 
			1,
			submesh.startIndexLoc,
			submesh.baseVertexLoc,
			0
		);

		// Trans Back to Render Targets
		TransResourceState(
			mCommandList,
			{ prevRenderTarget->GetColorResource(), mRenderTargets["trans"]->GetColorResource() },
			{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
			{ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET }
		);
	}
	*/

	// Resolve
	if (m4xMsaaState) {
		PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "Resolve MSAA");

		TransResourceState(
			mCommandList,
			{ nowColorRenderTarget->GetColorResource(), mRenderTargets["afterResolve"]->GetColorResource() },
			{ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET },
			{ D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST }
		);
		mCommandList->ResolveSubresource(
			mRenderTargets["afterResolve"]->GetColorResource(), 0, 
			nowColorRenderTarget->GetColorResource(), 0, 
			mRenderTargets["afterResolve"]->GetColorViewFormat()
		);
		TransResourceState(
			mCommandList,
			{ nowColorRenderTarget->GetColorResource(), mRenderTargets["afterResolve"]->GetColorResource() },
			{ D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST },
			{ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET }
		);

		nowColorRenderTarget = mRenderTargets["afterResolve"].get();
	}

	// FXAA
	if (mUseFXAA) {
		PIXScopedEvent(mCommandList.Get(), PIX_BLACK, "FXAA Postprocess");

		auto prevColorRenderTarget = nowColorRenderTarget;
		nowColorRenderTarget = mRenderTargets["fxaa"].get();
		nowDSRenderTarget = mRenderTargets["fxaa"].get();

		// Turn to Shader Resource
		TransRT2SR(mCommandList, { prevColorRenderTarget->GetColorResource() });

		// Set Root Signature
		mCommandList->SetGraphicsRootSignature(mRootSigns["fxaa"].Get());
		auto& signPI = mRootSignParamIndices["fxaa"];

		// Set PSO
		mCommandList->SetPipelineState(mPSOs["fxaa"].Get());

		// Assign SRV
		mCommandList->SetGraphicsRootDescriptorTable(
			signPI["texSR"], prevColorRenderTarget->GetSRVGPUHandle()
		);

		// Build Render Params
		ShadowPassRenderParams rps;
		rps.commandList = mCommandList;
		rps.RTVSize = mCbvSrvUavDescriptorSize;

		rps.passCBRootParamIndex = signPI["passCB"];
		rps.passCBBaseAddr = mFxaaConstantsBuffers->Resource()->GetGPUVirtualAddress();
		rps.passCBByteSize = mFxaaConstantsBuffers->getElementByteSize();

		// Draw
		DrawPass(
			rps,
			mFxaaConstants->getID(),
			nowColorRenderTarget,
			nowDSRenderTarget,
			{mBackgroundRenderItem}
		);

		// Trans back
		TransSR2RT(mCommandList, {prevColorRenderTarget->GetColorResource()});
	}

	// Copy to BackBuffer
	{
		CopyResource(
			mCommandList,
			mSwapChain->GetColorResource(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT,
			nowColorRenderTarget->GetColorResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
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
		mSwapChain->Swap();
	}

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}
