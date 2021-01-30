#include "SceneGraphApp.h"
#include "PIXHelper.h"

#include "SimulationConst.h"
#include "ShaderStruct.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

#include "CudaInterface.h"

void SceneGraphApp::InitSimulation()
{

}

void SceneGraphApp::Simulation(const GameTimer& gt) {
	CudaWait();
	// TODO Here just a test code
	//		Final Simulation should be far more complex

	auto generalMesh = Mesh::FindMeshByName("cloth");
	auto mesh = dynamic_cast<Mesh_cuda_interop*>(generalMesh);
	Vertex* verts = reinterpret_cast<Vertex*>(mesh->GetCudaDevicePtr());

	ClothSimulation(verts, mCudaStreamToRun, gt.TotalTime());

	CudaSignal();
	WaitForGPU();
}
