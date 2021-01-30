#include "SceneGraphApp.h"
#include "PIXHelper.h"

#include "SimulationConst.h"
#include "ShaderStruct.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

#include "CudaInterface.h"

void SceneGraphApp::Simulation(const GameTimer& gt) {
	// TODO Here just a test code
	//		Final Simulation should be far more complex

	auto generalMesh = Mesh::FindMeshByName("cloth");
	auto mesh = dynamic_cast<Mesh_cuda_interop*>(generalMesh);
	Vertex* verts = reinterpret_cast<Vertex*>(mesh->GetCudaDevicePtr());

	ClothSimulation(verts, mCudaStreamToRun, gt.TotalTime());

	// Add Gravity
	// static float v = 0.0f;
	// static float x = 0.0f;
	// static float g = 9.8f;
	// float dt = gt.DeltaTime();
	// v += g * dt;
	// x += v * dt;
	// mRootObject->SetTranslation(0, -x, 0);
}
