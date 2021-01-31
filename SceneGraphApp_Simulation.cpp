#include "SceneGraphApp.h"
#include "PIXHelper.h"

#include "SimulationConst.h"
#include "ShaderStruct.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

#include "CudaInterface.h"

void SceneGraphApp::InitSimulation()
{
	CudaWait();

	sim::InitClothSimulation(mCudaStreamToRun);

	CudaSignal();
	WaitForGPU();
}

void SceneGraphApp::Simulation(const GameTimer& gt) {
	CudaWait();
	// TODO Here just a test code
	//		Final Simulation should be far more complex

	auto generalMesh = Mesh::FindMeshByName("cloth");
	auto mesh = dynamic_cast<Mesh_cuda_interop*>(generalMesh);
	Vertex* verts = reinterpret_cast<Vertex*>(mesh->GetCudaDevicePtr());

	float timeRatio = sim::timestep / 0.5f;
	timeRatio = 1.2f; // set 2.0f means 2 faster

	static float timeCache = 0.0f;
	timeCache += gt.DeltaTime() * timeRatio;
	while (timeCache > sim::timestep) {
		sim::ClothSimulation(verts, mCudaStreamToRun, gt.TotalTime());

		timeCache -= sim::timestep;
	}
	sim::UpdateNormal(verts, mCudaStreamToRun);


	CudaSignal();
	WaitForGPU();
}
