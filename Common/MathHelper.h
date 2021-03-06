//***************************************************************************************
// MathHelper.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Helper math class.
//***************************************************************************************

#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>

class MathHelper
{
public:
	// Returns random float in [0, 1).
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(float a, float b)
	{
		return a + RandF()*(b-a);
	}

    static int Rand(int a, int b)
    {
        return a + rand() % ((b - a) + 1);
    }

	template<typename T>
	static T Min(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	template<typename T>
	static T Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}
	 
	template<typename T>
	static T Lerp(const T& a, const T& b, float t)
	{
		return a + (b-a)*t;
	}

	template<typename T>
	static T Clamp(const T& x, const T& low, const T& high)
	{
		return x < low ? low : (x > high ? high : x); 
	}

	// Returns the polar angle of the point (x,y) in [0, 2*PI).
	static float AngleFromXY(float x, float y);

	static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
	{
		return DirectX::XMVectorSet(
			radius*sinf(phi)*cosf(theta),
			radius*cosf(phi),
			radius*sinf(phi)*sinf(theta),
			1.0f);
	}

    static DirectX::XMMATRIX XM_CALLCONV InverseTranspose(DirectX::CXMMATRIX M)
	{
        DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(M);
        return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, M));
	}

	static DirectX::XMMATRIX GenNormalMat(DirectX::XMFLOAT4X4 mat) {
		mat._14 = 0.0f;
		mat._24 = 0.0f;
		mat._34 = 0.0f;
		mat._41 = 0.0f;
		mat._42 = 0.0f;
		mat._43 = 0.0f;
		mat._44 = 1.0f;
		DirectX::XMMATRIX normalModelMat = DirectX::XMLoadFloat4x4(&mat);
		return MathHelper::InverseTranspose(normalModelMat);
	}

	static DirectX::XMMATRIX XM_CALLCONV GenNormalMat(DirectX::CXMMATRIX mat) {
        DirectX::XMMATRIX A = mat;
		// Inverse-transpose is just applied to normals.  So zero out 
		// translation row so that it doesn't get into our inverse-transpose
		// calculation--we don't want the inverse-transpose of the translation.
		A.r[0].m128_f32[3] = 0.0f;
		A.r[1].m128_f32[3] = 0.0f;
		A.r[2].m128_f32[3] = 0.0f;
        A.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		return MathHelper::InverseTranspose(A);
	}

    static DirectX::XMFLOAT3X3 Identity3x3()
    {
		static DirectX::XMFLOAT3X3 I(
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f);

        return I;
    }

    static DirectX::XMFLOAT4X4 Identity4x4()
    {
        static DirectX::XMFLOAT4X4 I(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        return I;
    }

	static DirectX::XMFLOAT4 XMFLOAT3TO4(DirectX::XMFLOAT3 vec, float w) {
		return {
			vec.x,
			vec.y,
			vec.z,
			w
		};
	}

	static bool XMFLOAT3Parallel(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
		return a.x * b.y == b.x * a.y && a.x * b.z == b.x * a.z;
	}

	static DirectX::XMVECTOR RandUnitVec3();
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);

	static float AngleToRadius(float angle) {
		return angle / 180.0f * Pi;
	}

	static const float Infinity;
	static const float Pi;


};

