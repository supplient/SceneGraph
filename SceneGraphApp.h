#pragma once
#include <DirectXColors.h>

#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "RenderItem.h"

class SceneGraphApp : public D3DApp
{
public:
	SceneGraphApp(HINSTANCE hInstance);
	~SceneGraphApp();

	virtual bool Initialize()override;

	// Initialize
	void BuildInputLayout();
	void BuildRootSignature();
	void BuildShaders();
	void BuildPSOs();

	void BuildGeos();
	void BuildObjectConstantBuffers();

	void BuildRenderItems();

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;


	// Input Layout
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// Root Signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;

	// Shaders
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;

	// PSOs
	//    DESCs are saved for convenience.
	std::unordered_map<std::string, D3D12_GRAPHICS_PIPELINE_STATE_DESC> mPSODescs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	// Geometries
	std::unordered_map<std::string, std::shared_ptr<MeshGeometry>> mGeos;

	// Constant Buffers
	std::unique_ptr<UploadBuffer<ObjectConstants::Content>> mObjectConstantBuffers;

	// Render Items
	std::vector<std::shared_ptr<RenderItem>> mRenderItems;
};
