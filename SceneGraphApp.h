#pragma once
#include <DirectXColors.h>

#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "Light.h"
#include "RenderItem.h"
#include "Camera.h"

class SceneGraphApp : public D3DApp
{
public:
	SceneGraphApp(HINSTANCE hInstance);
	~SceneGraphApp();

	virtual bool Initialize()override;

	// Initialize
	// Init DirectX
	void BuildDescriptorHeaps();
	void BuildMidRenderTarget();

	// Init PSOs
	void BuildInputLayout();
	void BuildRootSignature();
	void BuildShaders();
	void BuildPSOs();

	// Init Scene
	void BuildLights();
	void BuildScene();

	// Init Testing
	void BuildTesting();

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

	// Transparency concerned
	// Just testing, so we do not sort them now.
	Microsoft::WRL::ComPtr<ID3D12Resource> mSumResource;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUAVDescHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUAVCPUDescHeap;

	// Camera
	Camera mCamera;

	// Middle Ranger Target
	DXGI_FORMAT mMidRenderTargetFormat;
	Microsoft::WRL::ComPtr<ID3D12Resource> mMidRenderTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE mMidRTVCPUHandle;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mMidRTVDescHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE mMidSRVCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mMidSRVGPUHandle;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mMidSRVDescHeap;

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
	std::shared_ptr<RenderItem> mBackgroundRenderItem = nullptr;

	// Constant Buffers
	std::unique_ptr<UploadBuffer<MaterialConstants::Content>> mMaterialConstantsBuffers;
	std::unique_ptr<UploadBuffer<ObjectConstants::Content>> mObjectConstantsBuffers;
	std::unique_ptr<UploadBuffer<PassConstants::Content>> mPassConstantsBuffers;
};
