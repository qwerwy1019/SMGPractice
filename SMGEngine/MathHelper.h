#pragma once
#include "stdafx.h"
#include <math.h>
#include "Exception.h"
#include <DirectXMath.h>

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

	// 충돌 처리
	static constexpr float NO_INTERSECTION = 10.f;
	static bool isColliding(float rv)
	{
		return 0 <= rv && rv < 5.f;
	}

	static bool isPointInTriangle(DirectX::FXMVECTOR t0,
		DirectX::FXMVECTOR t1,
		DirectX::FXMVECTOR t2,
		DirectX::CXMVECTOR p)
	{
		using namespace DirectX;
		XMVECTOR cross0 = XMVector3Normalize(XMVector3Cross((t0 - t1), (p - t1)));
		XMVECTOR cross1 = XMVector3Normalize(XMVector3Cross((t1 - t2), (p - t2)));
		XMVECTOR cross2 = XMVector3Normalize(XMVector3Cross((t2 - t0), (p - t0)));

		float d0 = XMVectorGetX(XMVector3Dot(cross0, cross1));
		float d1 = XMVectorGetX(XMVector3Dot(cross1, cross2));
		if (d0 < 0 ||
			d1 < 0)
		{
			return false;
		}
		return true;
	}
	static bool isPointInRectangle(DirectX::FXMVECTOR r0,
		DirectX::FXMVECTOR width,
		DirectX::FXMVECTOR height,
		DirectX::CXMVECTOR p)
	{
		using namespace DirectX;
		check(MathHelper::equal(XMVectorGetX(XMVector3Dot(width, height)), 0), "직사각형만 고려되었습니다.");

		XMVECTOR v = p - r0;
		float dotWidth = XMVectorGetX(XMVector3Dot(width, v));
		float dotHeight = XMVectorGetX(XMVector3Dot(height, v));

		if (dotWidth < 0 || XMVectorGetX(XMVector3Dot(width, width)) < dotWidth)
		{
			return false;
		}
		if (dotHeight < 0 || XMVectorGetX(XMVector3Dot(height, height)) < dotWidth)
		{
			return false;
		}

		return true;
	}

	static bool isLineIntersectLine(DirectX::FXMVECTOR f0,
		DirectX::FXMVECTOR t0,
		DirectX::FXMVECTOR f1,
		DirectX::FXMVECTOR t1,
		float& firstLineIntersectTime)
	{
		using namespace DirectX;
		//교차를 먼저 검사
		XMVECTOR cross0 = XMVector3Cross((f1 - f0), (t0 - f0));
		XMVECTOR cross1 = XMVector3Cross((t1 - f0), (t0 - f0));

		XMVECTOR cross2 = XMVector3Cross((t0 - f1), (t1 - f1));
		XMVECTOR cross3 = XMVector3Cross((f0 - f1), (t1 - f1));

		if (XMVectorGetX(XMVector3Dot(cross0, cross1)) >= 0)
		{
			return false;
		}
		if (XMVectorGetX(XMVector3Dot(cross2, cross3)) >= 0)
		{
			return false;
		}

		// 모든 점들이 한 평면 위에 있다면(실제로 교차한다면) 외적벡터의 방향은 같을 것이기 떄문에 x성분만 비교해도 될것이다. [7/14/2021 qwerw]
		firstLineIntersectTime = XMVectorGetX(XMVector3Cross((t1 - f1), (t0 - f0))) /
			XMVectorGetX(cross3);
		check(0.f < firstLineIntersectTime && firstLineIntersectTime < 1.f);
		return true;
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
		if (XMVector3Equal(plane, XMVectorZero()))
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
	static float XM_CALLCONV triangleIntersectRectangle(DirectX::FXMVECTOR t0,
											DirectX::FXMVECTOR t1,
											DirectX::FXMVECTOR t2,
											DirectX::CXMVECTOR r0,
											DirectX::CXMVECTOR width,
											DirectX::CXMVECTOR height,
											DirectX::CXMVECTOR velocity)
	{
		using namespace DirectX;
		// 만약 삼각형 세 점의 정사영이 사각형 안에 있으면, 그 점들을 검사한다.
		// 4*3개 순서쌍의 직선 사이 거리가 최소가 되면서 선분 사이에 있는 점들을 구한다.
		XMVECTOR trianglePlane = XMPlaneFromPoints(t0, t1, t2);
		XMVECTOR rectPlane = XMPlaneFromPointNormal(r0, XMVector3Normalize(XMVector3Cross(width, height)));
		float speed = XMVectorGetX(XMVector3Length(velocity));
		XMVECTOR velocityNormal = velocity / speed;

		XMVECTOR r1 = r0 + width;
		XMVECTOR r2 = r0 + width + height;
		XMVECTOR r3 = r0 + height;

		// todo [7/12/2021 qwerw]
		float minDistance = NO_INTERSECTION;
		std::array<XMVECTOR, 3> triangle = { t0, t1, t2 };
		std::array<XMVECTOR, 4> rect = { r0, r1, r2, r3 };
		std::array<XMVECTOR, 3> orthTriangle;
		std::array<XMVECTOR, 4> orthRect;

		for (int i = 0; i < 3; ++i)
		{
			check(MathHelper::equal(XMVectorGetW(triangle[i]), 1));
			float triangleDistance = XMVectorGetX(XMVector4Dot(velocityNormal, triangle[i]));
			orthTriangle[i] = triangle[i] + triangleDistance * velocityNormal;
			float rectDistance = XMVectorGetX(XMVector4Dot(rectPlane, triangle[i])) / XMVectorGetX(XMVector3Dot(rectPlane, velocityNormal));
			float distance = triangleDistance - rectDistance;
			if (speed < distance || distance < 0)
			{
				continue;
			}
			XMVECTOR rectPoint = triangle[i] + rectDistance * velocityNormal;
			if (isPointInRectangle(r0, width, height, rectPoint))
			{
				minDistance = std::min(minDistance, distance);
			}
		}

		for (int i = 0; i < 4; ++i)
		{
			check(MathHelper::equal(XMVectorGetW(rect[i]), 1));
			float triangleDistance = XMVectorGetX(XMVector4Dot(rectPlane, rect[i])) / XMVectorGetX(XMVector3Dot(trianglePlane, velocityNormal));;
			float rectDistance = XMVectorGetX(XMVector4Dot(velocityNormal, rect[i]));
			orthRect[i] = rect[i] + rectDistance * velocityNormal;
			float distance = triangleDistance - rectDistance;
			if (speed < distance || distance < 0)
			{
				continue;
			}
			if (isPointInTriangle(orthTriangle[0], orthTriangle[1], orthTriangle[2], orthRect[i]))
			{
				minDistance = std::min(minDistance, distance);
			}
		}

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				float intersect;
				if (isLineIntersectLine(orthRect[i], orthRect[(i + 1) % 4], orthTriangle[j], orthTriangle[(j + 1) % 3], intersect))
				{
					XMVECTOR p = rect[i] * intersect + rect[(j + 1) % 4] * (1.f - intersect);
					float distance = XMVectorGetX(XMVector4Dot(rectPlane, p));
					if (distance >= 0)
					{
						minDistance = std::min(minDistance, distance);
					}
				}
			}
		}

		return minDistance;
	}

	static float XM_CALLCONV triangleIntersectBox(DirectX::FXMVECTOR t0,
		DirectX::FXMVECTOR t1,
		DirectX::FXMVECTOR t2,
		DirectX::CXMVECTOR center,
		DirectX::CXMVECTOR boxX,
		DirectX::CXMVECTOR boxY,
		DirectX::CXMVECTOR boxZ,
		DirectX::CXMVECTOR velocity) noexcept
	{
		using namespace DirectX;

		float xDot = XMVectorGetX(XMVector3Dot(boxX, velocity));
		float yDot = XMVectorGetX(XMVector3Dot(boxY, velocity));
		float zDot = XMVectorGetX(XMVector3Dot(boxZ, velocity));
		// box data를 indexing 할수 있는 방법이 없을까? [7/13/2021 qwerw]
		// orthgonal triangle, velocity vector normalize(minor), edge check duplicated(true/false) [7/13/2021 qwerw]

		std::array<XMVECTOR, 3> orthTriangle;
		std::array<bool, 12> edgeChecked = { false, };
		std::array<bool, 8> vertexChecked = { false, };
		float rv = NO_INTERSECTION;
		if (xDot < -0.01)
		{
			rv = std::min(rv, triangleIntersectRectangle(
				t0, t1, t2,
				center - boxX + boxY + boxZ,
				-2.f * boxZ,
				-2.f * boxY,
				velocity));

		}
		else if (xDot > 0.01)
		{
			rv = std::min(rv, triangleIntersectRectangle(
				t0, t1, t2,
				center + boxX + boxY - boxZ,
				+2.f * boxZ,
				-2.f * boxY,
				velocity));
		}

		if (yDot < -0.01)
		{
			rv = std::min(rv, triangleIntersectRectangle(
				t0, t1, t2,
				center - boxX - boxY + boxZ,
				-2.f * boxZ,
				+2.f * boxX,
				velocity));
		}
		else if (yDot > 0.01)
		{
			rv = std::min(rv, triangleIntersectRectangle(
				t0, t1, t2,
				center - boxX + boxY - boxZ,
				+2.f * boxZ,
				+2.f * boxX,
				velocity));
		}

		if (zDot < -0.01)
		{
			rv = std::min(rv, triangleIntersectRectangle(
				t0, t1, t2,
				center - boxX + boxY - boxZ,
				+2.f * boxX,
				-2.f * boxY,
				velocity));
		}
		else if (zDot > 0.01)
		{
			rv = std::min(rv, triangleIntersectRectangle(
				t0, t1, t2,
				center - boxX - boxY + boxZ,
				+2.f * boxX,
				+2.f * boxY,
				velocity));
		}
		return rv;
	}

	static bool getRootOfQuadEquation(float a, float b, float c, float& r0, float& r1) noexcept
	{
		if (MathHelper::equal(a, 0))
		{
			return false;
		}
		float determinant = (b * b) - (4.f * a * c);
		if (determinant < 0)
		{
			return false;
		}
		float sqrtDet = std::sqrt(determinant);

		r0 = (-b - std::sqrt(determinant)) / (2 * a);
		r1 = (-b + std::sqrt(determinant)) / (2 * a);
		if (r1 < r0)
		{
			std::swap(r0, r1);
		}
		return true;
	}

	static float XM_CALLCONV triangleIntersectSphere(DirectX::FXMVECTOR t0,
		DirectX::FXMVECTOR t1,
		DirectX::FXMVECTOR t2,
		DirectX::CXMVECTOR sphereCenter,
		DirectX::CXMVECTOR sphereVelocity,
		float radius) noexcept
	{
		using namespace DirectX;
		std::array<XMVECTOR, 3> triangle = { t0, t1, t2 };
		const auto& plane = XMPlaneFromPoints(t0, t1, t2);
		if (XMVector3Equal(plane, XMVectorZero()))
		{
			return NO_INTERSECTION;
		}

		float planeDotVelocity = XMVectorGetX(XMVector3Dot(plane, sphereVelocity));
		if (planeDotVelocity > -FLT_EPSILON)
		{
			return NO_INTERSECTION;
		}

		check(MathHelper::equal(XMVectorGetW(sphereCenter), 1.f));
		float intersectTime = (radius - XMVectorGetX(XMVector4Dot(plane, sphereCenter))) / planeDotVelocity;
		if (intersectTime < 0.f || 1.f < intersectTime)
		{
			return NO_INTERSECTION;
		}

		XMVECTOR intersectPoint = sphereCenter + (sphereVelocity * intersectTime) + (radius * -plane);
		
		// 교점이 삼각형 내부에 있을 경우
		std::array<bool, 3> isIntersectionInside = { false, false, false };
		for (int i = 0; i < 3; ++i)
		{
			XMVECTOR cross = XMVector3Cross((triangle[i] - triangle[(i + 1) % 3]), (triangle[(i + 1) % 3] - intersectPoint));
			float d = XMVectorGetX(XMVector3Dot(cross, plane));
			if (d >= 0)
			{
				isIntersectionInside[i] = true;
			}
		}

		if (isIntersectionInside[0] && isIntersectionInside[1] && isIntersectionInside[2])
		{
			return intersectTime;
		}

		// 교점이 아예 멀리 있을 경우 밑에 부분을 스킵하는 내용을 추가할지 고민중 [7/12/2021 qwerw]
		intersectTime = NO_INTERSECTION;
		const float speedSq = XMVectorGetX(XMVector3LengthSq(sphereVelocity));
		for (int i = 0; i < 3; ++i)
		{
			if (isIntersectionInside[i]) // 반대편 꼭짓점 체크
			{
				float a = speedSq;
				float b = XMVectorGetX(XMVector3Dot(sphereCenter - triangle[(i + 2) % 3], sphereVelocity) * 2);
				float c = XMVectorGetX(XMVector3LengthSq(sphereCenter - triangle[(i + 2) % 3])) - radius * radius;

				float r0, r1;
				if (getRootOfQuadEquation(a, b, c, r0, r1))
				{
					if (r0 >= 0 && r0 <= 1.f)
					{
						intersectTime = std::min(intersectTime, r0);
					}
					else if (r1 >= 0 && r1 <= 1.f)
					{
						intersectTime = std::min(intersectTime, r1);
					}
				}
			}
			else // 모서리 체크
			{
				XMVECTOR edge = triangle[(i + 1) % 3] - triangle[i];
				XMVECTOR baseToVertex = triangle[i] - sphereCenter;

				float edgeSq = XMVectorGetX(XMVector3LengthSq(edge));
				float edgeDotVelocity = XMVectorGetX(XMVector3Dot(edge, sphereVelocity));
				float edgeDotBaseToVertex = XMVectorGetX(XMVector3Dot(edge, baseToVertex));
				float velocityDotBaseToVertex = XMVectorGetX(XMVector3Dot(sphereVelocity, baseToVertex));
				float baseToVertexSq = XMVectorGetX(XMVector3LengthSq(baseToVertex));

				float a = -speedSq + edgeDotVelocity * edgeDotVelocity / edgeSq;
				float b = 2.f * (velocityDotBaseToVertex - edgeDotVelocity * edgeDotBaseToVertex / edgeSq);
				float c = (1.f - baseToVertexSq) + edgeDotBaseToVertex * edgeDotBaseToVertex / edgeSq;
				
				float r0, r1;
				if (getRootOfQuadEquation(a, b, c, r0, r1))
				{
					float c = (XMVectorGetX(XMVector3Dot(edge, sphereVelocity))* r0 - XMVectorGetX(XMVector3Dot(edge, baseToVertex))) / edgeSq;
					if (c >= 0 && c <= 1.f)
					{
						intersectTime = std::min(intersectTime, r0);
					}
					else
					{
						c = (XMVectorGetX(XMVector3Dot(edge, sphereVelocity)) * r1 - XMVectorGetX(XMVector3Dot(edge, baseToVertex))) / edgeSq;
						if (c >= 0 && c <= 1.f)
						{
							intersectTime = std::min(intersectTime, r1);
						}						
					}
				}
			}
		}
		return intersectTime;
	}

};