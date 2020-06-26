#pragma once
#include <DirectXColors.h>

#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "Light.h"
#include "RenderItem.h"
#include "Camera.h"
#include "StaticDescriptorHeap.h"

class SceneGraphApp : public D3DApp
{
public:
	SceneGraphApp(HINSTANCE hInstance);
	~SceneGraphApp();

	virtual bool Initialize()override;

	// Initialize
	// Init DirectX
	void BuildDescriptorHeaps();

	// Init PSOs
	void BuildInputLayout();
	void BuildRootSignature();
	void BuildShaders();
	void BuildPSOs();

	// Init Scene
	void BuildLights();
	void BuildScene();

	// Init Scene Resources
	void BuildPassConstantBuffers();
	void UpdateLightsInPassConstantBuffers();
	void BuildGeos();
	void BuildMaterials();
	void BuildAndUpdateMaterialConstantBuffers();

	// Init Render Items
	void BuildRenderItems();
	
	// Init Render Item Resources
	void BuildObjectConstantBuffers();

	// ScreenSize Concerned Resources' Init
	void ResizeScreenUAVSRV();
	void ResizeOpaqueRenderTarget();
	void ResizeTransRenderTarget();
	void ResizeTransBlendRenderTarget();

	/// <summary>
	/// 把renderItemQueue中的RenderItem逐个绘制。不设置RenderTarget、RootSignature。会自动设置PSO。
	/// </summary>
	/// <param name="renderItemQueue">RenderItem队列</param>
	void DrawRenderItems(const std::vector<std::shared_ptr<RenderItem>>& renderItemQueue);

private:

	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	virtual void OnMouseWheel(WORD keyState, int delta, int x, int y)override;

	virtual void OnKeyUp(WPARAM vKey)override;
	virtual void OnKeyDown(WPARAM vKey)override;

	// Camera
	Camera mCamera;

	// Descriptor Heaps
	std::unique_ptr<StaticDescriptorHeap> mRTVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mCBVSRVUAVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mCBVSRVUAVCPUHeap = nullptr;

	// Opaque Render Target
	DXGI_FORMAT mOpaqueRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOpaqueRenderTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE mOpaqueRTVCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE mOpaqueSRVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mOpaqueSRVGPUHandle;

	// Transparent Render Target
	DXGI_FORMAT mTransRenderTargetFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTransRenderTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE mTransRTVCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE mTransSRVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mTransSRVGPUHandle;

	// Transparent Blend Render Target
	DXGI_FORMAT mTransBlendRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTransBlendRenderTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE mTransBlendRTVCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE mTransBlendSRVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mTransBlendSRVGPUHandle;

	// NCount UAV
	DXGI_FORMAT mNCountFormat = DXGI_FORMAT_R32_UINT;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNCountResource;
	D3D12_CPU_DESCRIPTOR_HANDLE mNCountUAVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mNCountUAVGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE mNCountUAVCPUHeapCPUHandle;

	// ZBuffer SRV
	DXGI_FORMAT mZBufferFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	Microsoft::WRL::ComPtr<ID3D12Resource> mZBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE mZBufferSRVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mZBufferSRVGPUHandle;

	// Input Layout
	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayouts;

	// Root Signature
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> mRootSigns;

	// Shaders
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;

	// PSOs
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	// Lights
	std::vector<DirectionLight> mDirLights;
	std::vector<PointLight> mPointLights;
	std::vector<SpotLight> mSpotLights;

	// Scene Constants
	std::unique_ptr<PassConstants> mPassConstants;

	// Geometries
	std::unordered_map<std::string, std::shared_ptr<MeshGeometry>> mGeos;

	// Material Constants
	std::unordered_map<std::string, std::shared_ptr<MaterialConstants>> mMtlConsts;

	// Object Constants
	std::unordered_map<std::string, std::shared_ptr<ObjectConstants>> mObjConsts;

	// Render Items
	std::vector<std::shared_ptr<RenderItem>> mRenderItemQueue;
	std::vector<std::shared_ptr<RenderItem>> mTransRenderItemQueue;
	std::shared_ptr<RenderItem> mBackgroundRenderItem = nullptr;

	// Constant Buffers
	std::unique_ptr<UploadBuffer<MaterialConstants::Content>> mMaterialConstantsBuffers;
	std::unique_ptr<UploadBuffer<ObjectConstants::Content>> mObjectConstantsBuffers;
	std::unique_ptr<UploadBuffer<PassConstants::Content>> mPassConstantsBuffers;
};
