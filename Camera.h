#pragma once
#include "../Common/d3dUtil.h"

class Camera {
public:
	DirectX::XMMATRIX GetViewMatrix() {
		DirectX::XMVECTOR eyePos = { 0.0f, 0.0f, -dist };
		DirectX::XMVECTOR verAxis = { 1.0f, 0.0f, 0.0f };
		DirectX::XMMATRIX horRotate = DirectX::XMMatrixRotationY(
			MathHelper::AngleToRadius(horAngle)
		);
		verAxis = DirectX::XMVector3Transform(verAxis, horRotate);
		DirectX::XMMATRIX verRotate = DirectX::XMMatrixRotationAxis(
			verAxis, MathHelper::AngleToRadius(verAngle)
		);
		eyePos = DirectX::XMVector3Transform(eyePos, horRotate);
		eyePos = DirectX::XMVector3Transform(eyePos, verRotate);

		DirectX::XMVECTOR focusPos = { 0.0f, 0.0f, 0.0f };
		
		return DirectX::XMMatrixLookAtLH(eyePos, focusPos, { 0.0f, 1.0f, 0.0f });
	}

	void DeltaVerAngle(float delta) {
		verAngle += delta;
		if (verAngle < -90.0f)
			verAngle = -90.0f;
		else if (verAngle > 90.0f)
			verAngle = 90.0f;
	}

	void DeltaHorAngle(float delta) {
		horAngle += delta;
		while (horAngle > 360.0f)
			horAngle -= 360.0f;
		while (horAngle < -360.0f)
			horAngle += 360.0f;
	}

private:
	float verAngle = 0.0f;
	float horAngle = 0.0f;
	float dist = 1.0f;
};
