#include "SceneGraphApp.h"

using namespace DirectX;

void SceneGraphApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mCamera.StartViewDragging(x, y);
	D3DApp::OnMouseDown(btnState, x, y);
}

void SceneGraphApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	if(mCamera.IsViewDragging())
		mCamera.EndViewDragging();
	D3DApp::OnMouseUp(btnState, x, y);
}

void SceneGraphApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if(mCamera.IsViewDragging())
		mCamera.UpdateViewDragging(x, y);
	D3DApp::OnMouseMove(btnState, x, y);
}

void SceneGraphApp::OnMouseWheel(WORD keyState, int delta, int x, int y)
{
	delta /= WHEEL_DELTA;
	mCamera.DeltaDist(-delta * Camera::DIST_PER_WHEEL);
}

void SceneGraphApp::OnKeyUp(WPARAM vKey)
{
	D3DApp::OnKeyUp(vKey);
}

void SceneGraphApp::OnKeyDown(WPARAM vKey)
{
	switch (vKey)
	{
	case VK_LEFT:
		mCamera.DeltaHorAngle(Camera::HOR_PER_KEY);
		break;
	case VK_RIGHT:
		mCamera.DeltaHorAngle(-Camera::HOR_PER_KEY);
		break;
	case VK_UP:
		mCamera.DeltaVerAngle(Camera::VER_PER_KEY);
		break;
	case VK_DOWN:
		mCamera.DeltaVerAngle(-Camera::VER_PER_KEY);
		break;
	default:
		break;
	}

	D3DApp::OnKeyDown(vKey);
}

