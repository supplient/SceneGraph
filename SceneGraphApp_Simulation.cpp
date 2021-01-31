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

#include <profileapi.h>
void SceneGraphApp::Simulation(const GameTimer& gt) {
	CudaWait();
	// TODO Here just a test code
	//		Final Simulation should be far more complex

	auto generalMesh = Mesh::FindMeshByName("cloth");
	auto mesh = dynamic_cast<Mesh_cuda_interop*>(generalMesh);
	Vertex* verts = reinterpret_cast<Vertex*>(mesh->GetCudaDevicePtr());

	float timeRatio = sim::timestep / 0.5f;
	timeRatio = 1.0f; // set 2.0f means 2 faster

	static float timeCache = 0.0f;
	timeCache += gt.DeltaTime() * timeRatio;

	int count = 0;
	
	__int64 prevTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);

	while (timeCache > sim::timestep) {
		sim::ClothSimulation(verts, mCudaStreamToRun, gt.TotalTime());

		timeCache -= sim::timestep;
		count++;
	}
	sim::UpdateNormal(verts, mCudaStreamToRun);

	__int64 newTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&newTime);
	double diffTime = (newTime - prevTime) * gt.GetSecondsPerCount();

	// if (diffTime > sim::timestep * 10) {
		// ::OutputDebugStringA(("\t\t\t\t" + std::to_string(gt.DeltaTime()) + " Delta\n").c_str());
		// ::OutputDebugStringA(("\t\t\t\t" + std::to_string(diffTime) + " Diff\n").c_str());
		// ::OutputDebugStringA(("\t\t\t\t" + std::to_string(count) + " Count\n").c_str());
	// }
	// else {
		// ::OutputDebugStringA((std::to_string(gt.DeltaTime()) + " Delta\n").c_str());
		// ::OutputDebugStringA((std::to_string(diffTime) + " Diff\n").c_str());
		::OutputDebugStringA((std::to_string(count) + " Count\n").c_str());
	// }


	CudaSignal();
	WaitForGPU();
}
