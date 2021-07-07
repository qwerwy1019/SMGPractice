#pragma once
#include "stdafx.h"
#include <math.h>
#include "Exception.h"

namespace MathHelper
{
	static constexpr float Pi = 3.1415926535f;
	static constexpr float Pi_DIV2 = 1.57079632679f;
	static constexpr DirectX::XMFLOAT4X4 Identity4x4{ 1.f, 0.f, 0.f, 0.f,
													  0.f, 1.f, 0.f, 0.f,
													  0.f, 0.f, 1.f, 0.f,
													  0.f, 0.f, 0.f, 1.f };

	static constexpr DirectX::XMFLOAT4X4 Zero4x4{ 0.f, 0.f, 0.f, 0.f,
													  0.f, 0.f, 0.f, 0.f,
													  0.f, 0.f, 0.f, 0.f,
													  0.f, 0.f, 0.f, 0.f };

	inline static float AddRadian(float lhs, float rhs)
	{
		return std::fmod(lhs + rhs, 2 * Pi);
	}

	static int Rand(const int a, const int b)
	{
		check(a < b, std::to_string(a) + " " + std::to_string(b));
		return a + rand() % ((b - a) + 1);
	}

	static float RandF(void)
	{
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	static float RandF(const float a, const float b)
	{
		check(a < b, std::to_string(a) + " " + std::to_string(b));
		return a + RandF() * (b - a);
	}

	static DirectX::XMVECTOR SphericalToCartesian(const float radius, const float phi, const float theta)
	{
		return DirectX::XMVectorSet(radius * sinf(phi) * cosf(theta), radius * cosf(phi), radius * sinf(phi) * sinf(theta), 1.f);
	}

	static bool equal(const DirectX::XMFLOAT4X4& lhs, const DirectX::XMFLOAT4X4& rhs)
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (lhs(i, j) != rhs(i, j))
				{
					return false;
				}
			}
		}
		return true;
	}

	static bool equal_d(double lhs, double rhs, double epsilon = DBL_EPSILON)
	{
		return abs(lhs - rhs) < epsilon;
	}
	static bool equal(float lhs, float rhs, float epsilon = FLT_EPSILON)
	{
		return abs(lhs - rhs) < epsilon;
	}
	static bool equal(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs, float epsilon = FLT_EPSILON)
	{
		return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon)
			&& equal(lhs.z, rhs.z, epsilon) && equal(lhs.w, rhs.w, epsilon);
	}
	static bool equal(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs, float epsilon = FLT_EPSILON)
	{
		return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon) && equal(lhs.z, rhs.z, epsilon);
	}

	static DirectX::XMFLOAT3 add(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
	{
		return DirectX::XMFLOAT3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
	}

	static DirectX::XMFLOAT4 add(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs)
	{
		return DirectX::XMFLOAT4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
	}

	static DirectX::XMFLOAT3 sub(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
	{
		return DirectX::XMFLOAT3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	}

	static DirectX::XMFLOAT4 sub(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs)
	{
		return DirectX::XMFLOAT4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
	}

	static DirectX::XMFLOAT3 mul(const DirectX::XMFLOAT3& lhs, const float rhs)
	{
		return DirectX::XMFLOAT3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
	}
	static DirectX::XMFLOAT3 div(const DirectX::XMFLOAT3& lhs, const float rhs)
	{
		return DirectX::XMFLOAT3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
	}
	static float lengthSq(const DirectX::XMFLOAT3& lhs)
	{
		return lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z;
	}

	static float length(const DirectX::XMFLOAT3& lhs)
	{
		return std::sqrt(lengthSq(lhs));
	}
	static void interpolateMatrix(const DirectX::XMFLOAT4X4& mat0, const DirectX::XMFLOAT4X4& mat1, float lerpPercent, DirectX::XMFLOAT4X4& matrix)
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				matrix.m[i][j] = mat0.m[i][j] * lerpPercent + mat1.m[i][j] * (1 - lerpPercent);
			}
		}
	}
	static void getWorldMatrix(const DirectX::XMFLOAT3& position,
								const DirectX::XMFLOAT3& direction,
								const DirectX::XMFLOAT3& upVector,
								const float size,
								DirectX::XMFLOAT4X4& outMatrix)
	{
		using namespace DirectX;

		XMMATRIX translation = XMMatrixTranslation(position.x, position.y, position.z);

		XMMATRIX rotation = XMMatrixSet(direction.y * upVector.z - direction.z * upVector.y,
			direction.z * upVector.x - direction.x * upVector.z,
			direction.x * upVector.y - direction.y * upVector.x, 0,
			upVector.x, upVector.y, upVector.z, 0,
			-direction.x, -direction.y, -direction.z, 0,
			0, 0, 0, 1);
		XMMATRIX scaling = XMMatrixScaling(size, size, size);

		XMStoreFloat4x4(&outMatrix, scaling * rotation * translation);
	}

	static DirectX::XMVECTOR XM_CALLCONV triangleIntersectLine(DirectX::FXMVECTOR t0,
														DirectX::FXMVECTOR t1,
														DirectX::FXMVECTOR t2,
														DirectX::CXMVECTOR from,
														DirectX::CXMVECTOR to,
														bool checkNormal) noexcept
	{
		using namespace DirectX;
		const auto& plane = XMPlaneFromPoints(t0, t1, t2);
		if (MathHelper::equal(XMVectorGetX(XMVector3LengthSq(plane)), 0.f))
		{
			return to;
		}

		if (checkNormal)
		{
			if (XMVectorGetX(XMVector3Dot(plane, to - from)) > 0)
			{
				return to;
			}
		}
		const auto& intersect = XMPlaneIntersectLine(plane, from, to);
		if (XMVectorGetX(XMVector3Dot(from - intersect, to - intersect)) > 0)
		{
			return to;
		}

		XMVECTOR cross0 = XMVector3Normalize(XMVector3Cross((t0 - t1), (intersect - t1)));
		XMVECTOR cross1 = XMVector3Normalize(XMVector3Cross((t1 - t2), (intersect - t2)));
		XMVECTOR cross2 = XMVector3Normalize(XMVector3Cross((t2 - t0), (intersect - t0)));

		float d0 = XMVectorGetX(XMVector3Dot(cross0, cross1));
		float d1 = XMVectorGetX(XMVector3Dot(cross1, cross2));
		if (d0 < 0 ||
			d1 < 0)
		{
			return to;
		}
		return intersect;
	}
};