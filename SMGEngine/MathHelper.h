#pragma once
#include "stdafx.h"
#include <math.h>
#include "Exception.h"
#include <DirectXMath.h>
#include <algorithm>

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
	static float dot(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
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

		XMMATRIX rotation = XMMatrixSet(
			direction.y * upVector.z - direction.z * upVector.y,
			direction.z * upVector.x - direction.x * upVector.z,
			direction.x * upVector.y - direction.y * upVector.x, 0,
			upVector.x, upVector.y, upVector.z, 0,
			-direction.x, -direction.y, -direction.z, 0,
			0, 0, 0, 1);
		XMMATRIX scaling = XMMatrixScaling(size, size, size);

		XMStoreFloat4x4(&outMatrix, scaling * rotation * translation);
	}

	// �浹 ó��
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
	static bool XM_CALLCONV isPointInRectangle(DirectX::FXMVECTOR r0,
		DirectX::FXMVECTOR width,
		DirectX::FXMVECTOR height,
		DirectX::CXMVECTOR p)
	{
		using namespace DirectX;
		check(MathHelper::equal(XMVectorGetX(XMVector3Dot(XMVector3Normalize(width), XMVector3Normalize(height))), 0, 0.0001f), "���簢���� ����Ǿ����ϴ�.");

		XMVECTOR v = p - r0;
		float dotWidth = XMVectorGetX(XMVector3Dot(width, v));
		float dotHeight = XMVectorGetX(XMVector3Dot(height, v));

		if (dotWidth < 0 || XMVectorGetX(XMVector3Dot(width, width)) < dotWidth)
		{
			return false;
		}
		if (dotHeight < 0 || XMVectorGetX(XMVector3Dot(height, height)) < dotHeight)
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
		//������ ���� �˻�
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
		XMVECTOR cross = XMVector3Cross((t1 - f1), (t0 - f0));
		firstLineIntersectTime = XMVectorGetX(XMVector3Length(cross3)) / XMVectorGetX(XMVector3Length(cross));
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

		if (false == isPointInTriangle(t0, t1, t2, intersect))
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
		// ���� �ﰢ�� �� ���� ���翵�� �簢�� �ȿ� ������, �� ������ �˻��Ѵ�.
		// 4*3�� �������� ���� ���� �Ÿ��� �ּҰ� �Ǹ鼭 ���� ���̿� �ִ� ������ ���Ѵ�.
		XMVECTOR trianglePlane = XMPlaneFromPoints(t0, t1, t2);
		float speed = XMVectorGetX(XMVector3Length(velocity));
		XMVECTOR velocityNormal = XMVectorSetW(velocity / speed, 0.f);

		if (XMVectorGetX(XMVector3Dot(trianglePlane, velocityNormal)) > -FLT_EPSILON)
		{
			return NO_INTERSECTION;
		}

		XMVECTOR rectPlane = XMPlaneFromPointNormal(r0, XMVector3Normalize(XMVector3Cross(height, width)));
		if (XMVectorGetX(XMVector3Dot(rectPlane, velocityNormal)) < FLT_EPSILON)
		{
			return NO_INTERSECTION;
		}

		float minDistance = speed * NO_INTERSECTION;
		std::array<XMVECTOR, 3> triangle = { XMVectorSetW(t0, 1.f), XMVectorSetW(t1, 1.f), XMVectorSetW(t2, 1.f) };
		std::array<XMVECTOR, 4> rect = { XMVectorSetW(r0, 1.f), XMVectorSetW(r0 + width, 1.f),
										XMVectorSetW(r0 + width + height, 1.f), XMVectorSetW(r0 + height, 1.f) };
		std::array<XMVECTOR, 3> orthTriangle;
		std::array<XMVECTOR, 4> orthRect;
		for (int i = 0; i < 3; ++i)
		{
			float triangleDistance = XMVectorGetX(XMVector4Dot(velocityNormal, triangle[i]));
			orthTriangle[i] = triangle[i] - triangleDistance * velocityNormal;
		}
		for (int i = 0; i < 4; ++i)
		{
			float rectDistance = XMVectorGetX(XMVector4Dot(velocityNormal, rect[i]));
			orthRect[i] = rect[i] - rectDistance * velocityNormal;
		}

		bool isNearDistance = false;
		for (int i = 0; i < 3; ++i)
		{
			float distance = XMVectorGetX(XMVector4Dot(rectPlane, triangle[i])) / XMVectorGetX(XMVector3Dot(rectPlane, velocityNormal));
			if (speed < distance || distance < 0)
			{
				continue;
			}
			isNearDistance = true;
			XMVECTOR rectPoint = triangle[i] - distance * velocityNormal;
			if (distance < minDistance && isPointInRectangle(r0, width, height, rectPoint))
			{
				minDistance = distance;
			}
		}
		if (false == isNearDistance)
		{
			return NO_INTERSECTION;
		}

		for (int i = 0; i < 4; ++i)
		{
			float distance = XMVectorGetX(XMVector4Dot(trianglePlane, rect[i])) / (-XMVectorGetX(XMVector3Dot(trianglePlane, velocityNormal)));
			if (speed < distance || distance < 0)
			{
				continue;
			}
			if (distance < minDistance && isPointInTriangle(orthTriangle[0], orthTriangle[1], orthTriangle[2], orthRect[i]))
			{
				minDistance = distance;
			}
		}

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				float intersect;
				if (isLineIntersectLine(orthRect[i], orthRect[(i + 1) % 4], orthTriangle[j], orthTriangle[(j + 1) % 3], intersect))
				{
					XMVECTOR p = XMVectorSetW(rect[i] * (1.f - intersect) + rect[(i + 1) % 4] * intersect, 1.f);
					float distance = XMVectorGetX(XMVector4Dot(trianglePlane, p)) / (-XMVectorGetX(XMVector3Dot(trianglePlane, velocityNormal)));
					if (0 <= distance && distance < minDistance)
					{
						minDistance = distance;
					}
				}
			}
		}

		return minDistance / speed;
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

		// box data�� indexing �Ҽ� �ִ� ����� ������? [7/13/2021 qwerw]
		// orthgonal triangle, velocity vector normalize(minor), edge check duplicated(true/false) [7/13/2021 qwerw]

		std::array<bool, 12> edgeChecked = { false, };
		std::array<bool, 8> vertexChecked = { false, };
		float rv = NO_INTERSECTION;
		
		// -x
		rv = std::min(rv, triangleIntersectRectangle(
			t0, t1, t2,
			center - boxX - boxY - boxZ,
			+2.f * boxZ,
			+2.f * boxY,
			velocity));
		// +x
		rv = std::min(rv, triangleIntersectRectangle(
			t0, t1, t2,
			center + boxX + boxY + boxZ,
			-2.f * boxY,
			-2.f * boxZ,
			velocity));
		// -y
		rv = std::min(rv, triangleIntersectRectangle(
			t0, t1, t2,
			center - boxX - boxY - boxZ,
			+2.f * boxX,
			+2.f * boxZ,
			velocity));
		// +y
		rv = std::min(rv, triangleIntersectRectangle(
			t0, t1, t2,
			center - boxX + boxY + boxZ,
			-2.f * boxZ,
			-2.f * boxX,
			velocity));
		// -z
		rv = std::min(rv, triangleIntersectRectangle(
			t0, t1, t2,
			center - boxX - boxY - boxZ,
			+2.f * boxY,
			+2.f * boxX,
			velocity));
		// +z
		rv = std::min(rv, triangleIntersectRectangle(
			t0, t1, t2,
			center + boxX + boxY + boxZ,
			-2.f * boxX,
			-2.f * boxY,
			velocity));

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

		float intersectTime = (radius - XMVectorGetX(XMVector4Dot(plane, XMVectorSetW(sphereCenter, 1.f)))) / planeDotVelocity;
		if (intersectTime < 0.f || 1.f < intersectTime)
		{
			return NO_INTERSECTION;
		}

		XMVECTOR intersectPoint = sphereCenter + (sphereVelocity * intersectTime) + (radius * -plane);
		
		// ������ �ﰢ�� ���ο� ���� ���
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

		// ������ �ƿ� �ָ� ���� ��� �ؿ� �κ��� ��ŵ�ϴ� ������ �߰����� ����� [7/12/2021 qwerw]
		intersectTime = NO_INTERSECTION;
		const float speedSq = XMVectorGetX(XMVector3LengthSq(sphereVelocity));
		for (int i = 0; i < 3; ++i)
		{
			if (isIntersectionInside[i]) // �ݴ��� ������ üũ
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
			else // �𼭸� üũ
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

	static float XM_CALLCONV getDeltaAngleToVector(DirectX::FXMVECTOR planeNormal, DirectX::FXMVECTOR originVectorNormal, DirectX::FXMVECTOR toVector) noexcept
	{
		using namespace DirectX;
		XMVECTOR toVectorOrth = toVector - XMVector3Dot(planeNormal, toVector) * planeNormal;
		if (XMVector3Equal(toVectorOrth, XMVectorZero()))
		{
			return 0.f;
		}
		toVectorOrth = XMVector3Normalize(toVectorOrth);
		float deltaAngle = acos(std::clamp(XMVectorGetX(XMVector3Dot(toVectorOrth, originVectorNormal)), -1.f, 1.f));
		if (XMVectorGetX(XMVector3Dot(planeNormal, XMVector3Cross(originVectorNormal, toVectorOrth))) > 0)
		{
			return deltaAngle;
		}
		else
		{
			return -deltaAngle;
		}
	}

	static DirectX::XMVECTOR XM_CALLCONV getQuaternion(DirectX::FXMVECTOR upVector, DirectX::FXMVECTOR direction) noexcept
	{
		using namespace DirectX;
		XMVECTOR rightVector = XMVector3Normalize(XMVector3Cross(upVector, direction));
		XMVECTOR upVectorApplied = XMVector3Cross(direction, rightVector);

		XMMATRIX rotationMatrix;
		rotationMatrix.r[0] = -rightVector;
		rotationMatrix.r[1] = upVectorApplied;
		rotationMatrix.r[2] = -direction;
		rotationMatrix.r[3] = XMVectorSet(0, 0, 0, 1);

		return XMQuaternionRotationMatrix(rotationMatrix);
	}

	static void XM_CALLCONV getRotatedAxis(DirectX::FXMVECTOR quaternion, 
										DirectX::XMVECTOR& upVector,
										DirectX::XMVECTOR& direction,
										DirectX::XMVECTOR& rightVector) noexcept
	{
		using namespace DirectX;
		XMMATRIX cameraRotateMatrix = XMMatrixRotationQuaternion(quaternion);
		rightVector = -cameraRotateMatrix.r[0];
		upVector = cameraRotateMatrix.r[1];
		direction = -cameraRotateMatrix.r[2];
	}
};