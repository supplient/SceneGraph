#pragma once
#include <DirectXColors.h>

#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "Light.h"
#include "RenderItem.h"
#include "Camera.h"
#include "StaticDescriptorHeap.h"
#include "RenderTarget.h"

class SceneGraphApp : public D3DApp
{
public:
	SceneGraphApp(HINSTANCE hInstance);
	~SceneGraphApp();

	virtual bool Initialize()override;

	// Initialize
	// Init DirectX
	/// <summary>
	/// 仅仅初始化RenderTarget，不建立资源，也不建立描述符
	/// </summary>
	void BuildRenderTargets();
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

	// Init Postprocess Resources
	void InitFxaa();

	// ScreenSize Concerned Resources' Init
	void ResizeScreenUAVSRV();
	void ResizeRenderTargets();
	void ResizeFxaa();

	/// <summary>
	/// 把renderItemQueue中的RenderItem逐个绘制。不设置RenderTarget、RootSignature。会自动设置PSO。
	/// </summary>
	/// <param name="renderItemQueue">RenderItem队列</param>
	void DrawRenderItems(const std::vector<std::shared_ptr<RenderItem>>& renderItemQueue);

private:

	virtual void OnMsaaStateChange()override;
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual std::vector<CD3DX12_STATIC_SAMPLER_DESC> GetStaticSamplers();

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	virtual void OnMouseWheel(WORD keyState, int delta, int x, int y)override;

	virtual void OnKeyUp(WPARAM vKey)override;
	virtual void OnKeyDown(WPARAM vKey)override;

	// Camera
	Camera mCamera;

	// Descriptor Heaps
	std::unique_ptr<StaticDescriptorHeap> mDSVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mRTVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mCBVSRVUAVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mCBVSRVUAVCPUHeap = nullptr;

	// Render Targets
	std::unordered_map<std::string, std::unique_ptr<RenderTarget>> mRenderTargets;

	// NCount UAV
	DXGI_FORMAT mNCountFormat = DXGI_FORMAT_R32_UINT;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNCountResource;
	D3D12_CPU_DESCRIPTOR_HANDLE mNCountUAVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mNCountUAVGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE mNCountUAVCPUHeapCPUHandle;

	// ZBuffer SRV
	DXGI_FORMAT mZBufferResourceFormat = DXGI_FORMAT_R32_TYPELESS;
	DXGI_FORMAT mZBufferViewFormat = DXGI_FORMAT_R32_FLOAT;
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

	// PostProcess Constants
	std::unique_ptr<FxaaConstants> mFxaaConstants;

	// Render Items
	std::vector<std::shared_ptr<RenderItem>> mRenderItemQueue;
	std::vector<std::shared_ptr<RenderItem>> mTransRenderItemQueue;
	std::shared_ptr<RenderItem> mBackgroundRenderItem = nullptr;

	// Constant Buffers
	std::unique_ptr<UploadBuffer<MaterialConstants::Content>> mMaterialConstantsBuffers;
	std::unique_ptr<UploadBuffer<ObjectConstants::Content>> mObjectConstantsBuffers;
	std::unique_ptr<UploadBuffer<PassConstants::Content>> mPassConstantsBuffers;
	std::unique_ptr<UploadBuffer<FxaaConstants::Content>> mFxaaConstantsBuffers;
};
