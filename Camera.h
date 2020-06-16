#pragma once
#include "../Common/d3dUtil.h"

class Camera {
public:
	static constexpr float VER_PER_KEY = 4.0f;
	static constexpr float HOR_PER_KEY = 4.0f;
	static constexpr float ANGLE_PER_X = 0.5;
	static constexpr float ANGLE_PER_Y = 0.5;

	DirectX::XMMATRIX GetViewMatrix() {
		// Cal actual horAngle & verAngle
		float horAngle = mHorAngle;
		float verAngle = mVerAngle;
		if (IsViewDragging()) {
			auto mouseDelta = CalAngleByMousePos();
			horAngle += mouseDelta[0];
			verAngle += mouseDelta[1];
			horAngle = LimitHorAngle(horAngle);
			verAngle = LimitVerAngle(verAngle);
		}
		float horRadius = MathHelper::AngleToRadius(horAngle);
		float verRadius = MathHelper::AngleToRadius(verAngle);

		// Cal eyePos
		DirectX::XMVECTOR eyePos = { 0.0f, 0.0f, -mDist };
		DirectX::XMVECTOR verAxis = { 1.0f, 0.0f, 0.0f };
		DirectX::XMMATRIX horRotate = DirectX::XMMatrixRotationY(horRadius);
		verAxis = DirectX::XMVector3Transform(verAxis, horRotate);
		DirectX::XMMATRIX verRotate = DirectX::XMMatrixRotationAxis(verAxis, verRadius);
		eyePos = DirectX::XMVector3Transform(eyePos, horRotate);
		eyePos = DirectX::XMVector3Transform(eyePos, verRotate);

		// Cal focusPos
		DirectX::XMVECTOR focusPos = { 0.0f, 0.0f, 0.0f };
		
		// Cal View Matrix
		return DirectX::XMMatrixLookAtLH(eyePos, focusPos, { 0.0f, 1.0f, 0.0f });
	}

	void DeltaHorAngle(float delta) {
		mHorAngle += delta;
		mHorAngle = LimitHorAngle(mHorAngle);
	}

	void DeltaVerAngle(float delta) {
		mVerAngle += delta;
		mVerAngle = LimitVerAngle(mVerAngle);
	}

	bool IsViewDragging() { return mViewDragging; }

	void StartViewDragging(int x, int y) {
		mViewDragging = true;
		mStartMousePos[0] = x;
		mStartMousePos[1] = y;
		UpdateViewDragging(x, y);
	}

	void UpdateViewDragging(int x, int y) {
		mNowMousePos[0] = x;
		mNowMousePos[1] = y;
	}
	
	void EndViewDragging() {
		mViewDragging = false;
		auto mouseDelta = CalAngleByMousePos();
		DeltaHorAngle(mouseDelta[0]);
		DeltaVerAngle(mouseDelta[1]);
	}

private:
	float mHorAngle = 0.0f;
	float mVerAngle = 0.0f;
	float mDist = 1.0f;

	float mViewDragging = false;
	std::array<int, 2> mStartMousePos;
	std::array<int, 2> mNowMousePos;

	std::array<float, 2> CalAngleByMousePos() {
		return {
			(mNowMousePos[0] - mStartMousePos[0]) * ANGLE_PER_X,
			(mNowMousePos[1] - mStartMousePos[1]) * ANGLE_PER_Y,
		};
	}

	float LimitHorAngle(float angle) {
		while (angle > 360.0f)
			angle -= 360.0f;
		while (angle < -360.0f)
			angle += 360.0f;
		return angle;
	}

	float LimitVerAngle(float angle) {
		if (angle < -90.0f)
			angle = -90.0f;
		else if (angle > 90.0f)
			angle = 90.0f;
		return angle;
	}
};
