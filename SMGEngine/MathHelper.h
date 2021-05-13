#pragma once
#include <math.h>
#include <DirectXMath.h>
#include "Exception.h"

class MathHelper
{
public:
	static constexpr float Pi = 3.1415926535f;
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

	static bool equal(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs, float epsilon = FLT_EPSILON)
	{
		return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon)
			&& equal(lhs.z, rhs.z, epsilon) && equal(lhs.w, rhs.w, epsilon);
	}
	static bool equal(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs, float epsilon = FLT_EPSILON)
	{
		return equal(lhs.x, rhs.x, epsilon) && equal(lhs.y, rhs.y, epsilon) && equal(lhs.z, rhs.z, epsilon);
	}
	static bool equal(float lhs, float rhs, float epsilon = FLT_EPSILON)
	{
		return abs(lhs - rhs) < epsilon;
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

	static float distanceSq(const DirectX::XMFLOAT3& lhs)
	{
		return lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z;
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
};