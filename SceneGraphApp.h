#pragma once
#include <DirectXColors.h>
#include <functional>

#include "Common/d3dApp.h"
#include "Common/UploadBuffer.h"
#include "Object.h"
#include "Light.h"
#include "RenderItem.h"
#include "Camera.h"
#include "StaticDescriptorHeap.h"
#include "RenderTarget.h"
#include "UnorderedAccessBuffer.h"
#include "Texture.h"

class SceneGraphApp : public D3DApp
{
public:
	SceneGraphApp(HINSTANCE hInstance);
	~SceneGraphApp();

	virtual bool Initialize()override;

	// Initialize

	// Init Scene
	void BuildObjects();
	void BuildLights();
	void BuildLightShadowConstantBuffers();
	void BuildTextures();

	// Init DirectX
	void BuildUABs();
	void BuildRenderTargets(); // 仅仅初始化RenderTarget，不建立资源，也不建立描述符
	void BuildDescriptorHeaps();

	// Init PSOs
	void BuildInputLayout();
	void BuildRootSignature();
	void BuildShaders();
	void BuildPSOs();

	// Init Scene Resources
	void LoadTextures();
	void BuildPassConstants();
	void BuildPassConstantBuffers();
	void UpdateLightsInPassConstantBuffers();
	void BuildGeos();
	void BuildMaterialConstants();
	void BuildAndUpdateMaterialConstantBuffers();
	
	// Init Render Item Resources
	void BuildObjectConstantBuffers();

	// Init Postprocess Resources
	void InitHbao();
	void InitFxaa();

	// ScreenSize Concerned Resources' Init
	void ResizeScreenUAVSRV();
	void ResizeRenderTargets();
	void ResizeShadowRenderTargets();
	void ResizeFxaa();

	/// <summary>
	/// 把renderItemQueue中的RenderItem逐个绘制。不设置RenderTarget、RootSignature。会自动设置PSO。
	/// </summary>
	/// <param name="renderItemQueue">RenderItem队列</param>
	/// <param name="rootSignParamIndices">用于标识各个根参数的序号</param>
	void DrawRenderItems(
		const std::vector<std::shared_ptr<RenderItem>>& renderItemQueue,
		std::unordered_map<std::string, UINT>& rootSignParamIndices
	);

private:

	virtual void OnMsaaStateChange()override;
	virtual void OnFxaaStateChange();
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

	// Settings
	bool mUseFXAA = false;
	void UpdateFXAAState(bool newState);
	// TODO The solutions of shadow mapping should be dynamic
	//		Here we hard-encoding them for convenience
	static const UINT SHADOW_MAPPING_WIDTH = 1024;
	static const UINT SHADOW_MAPPING_HEIGHT = 1024;

	// Viewports & ScissorRects
	D3D12_VIEWPORT mShadowScreenViewport;
    D3D12_RECT mShadowScissorRect;

	// Camera
	Camera mCamera;

	// Descriptor Heaps
	std::unique_ptr<StaticDescriptorHeap> mDSVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mRTVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mCBVSRVUAVHeap = nullptr;
	std::unique_ptr<StaticDescriptorHeap> mCBVSRVUAVCPUHeap = nullptr;

	// Render Targets
	std::unordered_map<std::string, std::unique_ptr<SingleRenderTarget>> mRenderTargets;
	std::unique_ptr<SwapChainRenderTarget> mSwapChain;

	// Unordered Access Buffers
	std::unordered_map<std::string, std::unique_ptr<UnorderedAccessBuffer>> mUABs;

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
	std::unordered_map<std::string, std::unordered_map<std::string, UINT>> mRootSignParamIndices;

	// Shaders
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;

	// PSOs
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	// Objects
	std::shared_ptr<Object> mRootObject;

	// Lights
	std::vector<DirectionLight> mDirLights;
	std::vector<PointLight> mPointLights;
	std::vector<SpotLight> mSpotLights;
	std::vector<RectLight> mRectLights;

	// Light Shadows
	D3D12_GPU_DESCRIPTOR_HANDLE mDirShadowTexGPUHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE mDirShadowTexCPUHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE mSpotShadowTexGPUHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE mSpotShadowTexCPUHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE mPointShadowTexGPUHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE mPointShadowTexCPUHandleStart;

	// Textures
	std::unordered_map<std::string, std::unique_ptr<ResourceTexture>> mResourceTextures;
	D3D12_GPU_DESCRIPTOR_HANDLE mTexGPUHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE mTexCPUHandleStart;

	// Scene Constants
	std::unique_ptr<PassConstants> mPassConstants;

	// Geometries
	std::unordered_map<std::string, std::shared_ptr<MeshGeometry>> mGeos;

	// Material Constants
	std::unordered_map<std::string, std::shared_ptr<MaterialConstants>> mMtlConsts;

	// PostProcess Constants
	std::unique_ptr<HbaoConstants> mHbaoConstants;
	std::unique_ptr<FxaaConstants> mFxaaConstants;

	// Render Items
	std::vector<std::shared_ptr<RenderItem>> mOpaqueRenderItemQueue;
	std::vector<std::shared_ptr<RenderItem>> mTransRenderItemQueue;
	std::shared_ptr<RenderItem> mBackgroundRenderItem = nullptr;

	// Constant Buffers
	std::unique_ptr<UploadBuffer<MaterialConstants::Content>> mMaterialConstantsBuffers;
	std::unique_ptr<UploadBuffer<Object::Content>> mObjectConstantsBuffers;
	std::unique_ptr<UploadBuffer<PassConstants::Content>> mPassConstantsBuffers;
	std::unique_ptr<UploadBuffer<HbaoConstants::Content>> mHbaoConstantsBuffers;
	std::unique_ptr<UploadBuffer<FxaaConstants::Content>> mFxaaConstantsBuffers;
	std::unique_ptr<UploadBuffer<ShadowPassConstants::Content>> mShadowPassConstantsBuffers;
};
