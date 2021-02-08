#include "stdafx.h"
#include "GeometryGenerator.h"
#include <utility>
#include <array>
#include <fstream>
#include "D3DUtil.h"

using namespace DirectX;

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(
	float bottomRadius,
	float topRadius,
	float height,
	uint32_t sliceCount,
	uint32_t stackCount)
{
	check(height > 0, "비정상입니다.");
	check(((uint64_t)stackCount + 1) * ((uint64_t)sliceCount + 1) + ((uint64_t)sliceCount * 3) < std::numeric_limits<uint16_t>::max()
		, "uint16 인덱스 범위를 넘어가는 정점 갯수입니다.");
	check(((uint64_t)sliceCount * stackCount + sliceCount) * 6 < std::numeric_limits<uint32_t>::max()
		, "오버플로우가 납니다.");
	
	MeshData result;
	result._vertices.reserve((stackCount + 1) * (sliceCount + 1) + (sliceCount * 3));
	result._indices.reserve((sliceCount * stackCount + sliceCount) * 6);

	float heightPerStack = height / stackCount;
	float rDiff = (bottomRadius - topRadius);
	float deltaR = rDiff / stackCount;
	float deltaTheta = 2.f * XM_PI / sliceCount;
	float zeroHeight = -height * 0.5f;

	// 기둥 부분
	for (uint32_t i = 0; i <= stackCount; ++i)
	{
		float y = zeroHeight + heightPerStack * i;
		float r = bottomRadius - deltaR * i;
		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			VertexData vertex;
			float c = cosf(deltaTheta * j);
			float s = sinf(deltaTheta * j);
			vertex._position = { r * c, y, r * s };
			vertex._tangentU = { -s, 0, c };
			vertex._textureUV = { static_cast<float>(j) / sliceCount, 1.f - static_cast<float>(i) / stackCount };

			XMFLOAT3 biTangent = { c * rDiff, -height, s * rDiff };
			XMVECTOR T = XMLoadFloat3(&vertex._tangentU);
			XMVECTOR B = XMLoadFloat3(&biTangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex._normal, N);
			result._vertices.push_back(vertex);
		}
	}

	for (uint32_t i = 0; i < stackCount; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			const int p0 = i * (sliceCount + 1) + j;
			const int p1 = p0 + (sliceCount + 1);
			const int p2 = p1 + 1;
			const int p3 = p0 + 1;

			result._indices.push_back(p0);
			result._indices.push_back(p1);
			result._indices.push_back(p2);

			result._indices.push_back(p2);
			result._indices.push_back(p3);
			result._indices.push_back(p0);
		}
	}
	
	// 윗면 원 부분
	const uint16_t topBaseIndex = result._vertices.size();
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		VertexData vertex;
		float x = topRadius * cosf(deltaTheta * i);
		float z = topRadius * sinf(deltaTheta * i);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		result._vertices.push_back(VertexData(x, -zeroHeight, z, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, u, v));
	}
	result._vertices.push_back(VertexData(0.f, -zeroHeight, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.5f, 0.5f));

	const uint16_t topCenterIndex = result._vertices.size() - 1;
	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		result._indices.push_back(topCenterIndex);
		result._indices.push_back(topBaseIndex + i + 1);
		result._indices.push_back(topBaseIndex + i);
	}

	// 아랫면 원 부분
	const uint16_t bottomBaseIndex = result._vertices.size();
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		VertexData vertex;
		float x = bottomRadius * cosf(deltaTheta * i);
		float z = bottomRadius * sinf(deltaTheta * i);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		result._vertices.push_back(VertexData(x, zeroHeight, z, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f, u, v));
	}
	result._vertices.push_back(VertexData(0.f, zeroHeight, 0.f, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f, 0.5f, 0.5f));

	const uint16_t bottomCenterIndex = result._vertices.size() - 1;
	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		result._indices.push_back(bottomCenterIndex);
		result._indices.push_back(bottomBaseIndex + i);
		result._indices.push_back(bottomBaseIndex + i + 1);
	}
	return result;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
	check(radius > 0, "비정상입니다.");
	check(((uint64_t)stackCount - 1) * ((uint64_t)sliceCount + 1) + ((uint64_t)sliceCount * 3) < std::numeric_limits<uint16_t>::max()
		, "uint16 인덱스 범위를 넘어가는 정점 갯수입니다.");
	check(((uint64_t)sliceCount * stackCount - sliceCount) * 6 < std::numeric_limits<uint32_t>::max()
		, "오버플로우가 납니다.");

	MeshData result;
	result._vertices.reserve((stackCount - 1) * (sliceCount + 1) + (sliceCount * 3));
	result._indices.reserve((sliceCount * stackCount - sliceCount) * 6);

	float deltaPhi = XM_PI / stackCount;
	float deltaTheta = XM_2PI / sliceCount;

	// 점
	result._vertices.emplace_back(0.f, radius, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f);
	for (uint32_t i = 1; i < stackCount; ++i)
	{
		float phi = deltaPhi * i;
		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			VertexData vertex;
			float theta = deltaTheta * j;
			float y = radius * cosf(phi);
			float x = radius * sinf(phi) * cosf(theta);
			float z = radius * sinf(phi) * sinf(theta);
			vertex._position = { x, y, z };
			vertex._normal = { sinf(phi) * cosf(theta), cosf(phi), sinf(phi) * sinf(theta) };
			vertex._tangentU = { -sinf(theta), 0.f, cosf(theta) };

			vertex._textureUV = { theta / XM_2PI, phi / XM_PI };

			result._vertices.push_back(vertex);
		}
	}
	result._vertices.emplace_back(0.f, -radius, 0.f, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);

	// 인덱스
	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		result._indices.push_back(0);
		result._indices.push_back(i + 2);
		result._indices.push_back(i + 1);
	}
	for (uint32_t i = 0; i < stackCount; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			const int p0 = i * (sliceCount + 1) + j + 1;
			const int p1 = p0 + 1;
			const int p2 = p1 + sliceCount + 1;
			const int p3 = p2 - 1;

			result._indices.push_back(p0);
			result._indices.push_back(p1);
			result._indices.push_back(p3);

			result._indices.push_back(p1);
			result._indices.push_back(p2);
			result._indices.push_back(p3);
		}
	}
	check(result._vertices.size() - 1 >= 0, "underflow");
	const uint32_t bottemIndex = result._vertices.size() - 1;
	check(bottemIndex - sliceCount - 1 >= 0, "underflow");
	const uint32_t baseIndex = bottemIndex - sliceCount - 1;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		result._indices.push_back(bottemIndex);
		result._indices.push_back(i + baseIndex);
		result._indices.push_back(i + baseIndex + 1);
	}

	return result;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(float radius, uint32_t subDivisions)
{
	MeshData result;

	check(subDivisions < 7, "division level이 너무 큽니다. 6으로 조정됩니다.");
	subDivisions = std::min<uint32_t>(subDivisions, 6);

	const float X = 0.525731f;
	const float Z = 0.850651f;
	std::array <XMFLOAT3,12> defaultPos =
	{
		XMFLOAT3(-X, 0.0f, Z),		XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z),		XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X),		XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X),		XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f),		XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f),		XMFLOAT3(-Z, -X, 0.0f),
	};
	std::array<uint32_t, 60> defaultIndex =
	{
		1,4,0, 4,9,0, 4,5,9, 8,5,4, 1,8,4,
		1,10,8, 10,3,8, 8,3,5, 3,2,5, 3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9, 11,2,7,
	};

	result._vertices.resize(12);
	result._indices.assign(defaultIndex.begin(), defaultIndex.end());

	for (int i = 0; i < defaultPos.size(); ++i)
	{
		result._vertices[i]._position = defaultPos[i];
	}
	for (uint32_t i = 0; i < subDivisions; ++i)
	{
		SubDivide(result);
	}

	for (int i = 0; i < result._vertices.size(); ++i)
	{
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&result._vertices[i]._position));
		XMVECTOR pos = n * radius;

		XMStoreFloat3(&result._vertices[i]._position, pos);
		XMStoreFloat3(&result._vertices[i]._normal, n);

		float phi = acosf(result._vertices[i]._position.y / radius);
		float theta = atan2f(result._vertices[i]._position.z, result._vertices[i]._position.x);

		if (theta < 0.f)
		{
			theta += XM_2PI;
		}

		result._vertices[i]._tangentU.x = -sinf(theta);
		result._vertices[i]._tangentU.y = 0.f;
		result._vertices[i]._tangentU.z = cos(theta);

		result._vertices[i]._textureUV.x = theta / XM_2PI;
		result._vertices[i]._textureUV.y = phi / XM_PI;

	}
	return result;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32_t m, uint32_t n)
{
	MeshData result;

	check(m * n <= std::numeric_limits<uint16_t>::max(), "인덱스가 범위를 넘어갑니다.");
	
	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.f / (n - 1);
	float dv = 1.f / (m - 1);

	result._vertices.resize(m * n);
	for (uint32_t i = 0; i < m; ++i)
	{
		float z = (depth / 2.0f) - (i * dz);
		for (uint32_t j = 0; j < n; ++j)
		{
			float x = -(width / 2.0f) + (j * dx);
			result._vertices[i * n + j]._position = XMFLOAT3(x, 0.f, z);
			result._vertices[i * n + j]._normal = XMFLOAT3(0.f, 1.f, 0.f);
			result._vertices[i * n + j]._tangentU = XMFLOAT3(1.f, 0.f, 0.f);
			result._vertices[i * n + j]._textureUV = XMFLOAT2(j * du, i * dv);
		}
	}
	result._indices.resize((n - 1) * (m - 1) * 6);
	uint32_t k = 0;
	for (uint32_t i = 0; i < m - 1; ++i)
	{
		for (uint32_t j = 0; j < n - 1; ++j, k+=6)
		{
			result._indices[k] = i * n + j;
			result._indices[k + 1] = i * n + j + 1;
			result._indices[k + 2] = (i + 1) * n + j;

			result._indices[k + 3] = (i + 1) * n + j;
			result._indices[k + 4] = i * n + j + 1;
			result._indices[k + 5] = (i + 1) * n + j + 1;
		}
	}
	return result;
}

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float width, float height, float depth, uint32_t subDivisions)
{
	MeshData result;

	VertexData v[24];

	float halfWidth = 0.5f * width;
	float halfHeight = 0.5f * height;
	float halfDepth = 0.5f * depth;

	v[0] = VertexData(-halfWidth, -halfHeight, -halfDepth, 0.f, 0.f, -1.f, 1.f, 0.f, 0.f, 0.f, 1.f);
	v[1] = VertexData(-halfWidth, +halfHeight, -halfDepth, 0.f, 0.f, -1.f, 1.f, 0.f, 0.f, 0.f, 0.f);
	v[2] = VertexData(+halfWidth, +halfHeight, -halfDepth, 0.f, 0.f, -1.f, 1.f, 0.f, 0.f, 1.f, 0.f);
	v[3] = VertexData(+halfWidth, -halfHeight, -halfDepth, 0.f, 0.f, -1.f, 1.f, 0.f, 0.f, 1.f, 1.f);

	v[4] = VertexData(+halfWidth, -halfHeight, +halfDepth, 0.f, 0.f, 1.f, -1.f, 0.f, 0.f, 0.f, 1.f);
	v[5] = VertexData(+halfWidth, +halfHeight, +halfDepth, 0.f, 0.f, 1.f, -1.f, 0.f, 0.f, 0.f, 0.f);
	v[6] = VertexData(-halfWidth, +halfHeight, +halfDepth, 0.f, 0.f, 1.f, -1.f, 0.f, 0.f, 1.f, 0.f);
	v[7] = VertexData(-halfWidth, -halfHeight, +halfDepth, 0.f, 0.f, 1.f, -1.f, 0.f, 0.f, 1.f, 1.f);

	v[8] = VertexData(-halfWidth, -halfHeight, +halfDepth, -1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 1.f);
	v[9] = VertexData(-halfWidth, +halfHeight, +halfDepth, -1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f);
	v[10] = VertexData(-halfWidth, +halfHeight, -halfDepth, -1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 1.f, 0.f);
	v[11] = VertexData(-halfWidth, -halfHeight, -halfDepth, -1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 1.f, 1.f);

	v[12] = VertexData(+halfWidth, -halfHeight, -halfDepth, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f);
	v[13] = VertexData(+halfWidth, +halfHeight, -halfDepth, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f);
	v[14] = VertexData(+halfWidth, +halfHeight, +halfDepth, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f);
	v[15] = VertexData(+halfWidth, -halfHeight, +halfDepth, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f);

	v[16] = VertexData(-halfWidth, +halfHeight, -halfDepth, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);
	v[17] = VertexData(-halfWidth, +halfHeight, +halfDepth, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f);
	v[18] = VertexData(+halfWidth, +halfHeight, +halfDepth, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f);
	v[19] = VertexData(+halfWidth, +halfHeight, -halfDepth, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f);

	v[20] = VertexData(-halfWidth, -halfHeight, -halfDepth, 0.f, -1.f, 0.f, -1.f, 0.f, 0.f, 1.f, 1.f);
	v[21] = VertexData(+halfWidth, -halfHeight, -halfDepth, 0.f, -1.f, 0.f, -1.f, 0.f, 0.f, 0.f, 1.f);
	v[22] = VertexData(+halfWidth, -halfHeight, +halfDepth, 0.f, -1.f, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f);
	v[23] = VertexData(-halfWidth, -halfHeight, +halfDepth, 0.f, -1.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f);

	result._vertices.assign(&v[0], &v[24]);
	uint32_t i[36] =
	{
		0, 1, 2, 0, 2, 3,
		4, 5, 6, 4, 6, 7,
		8, 9, 10, 8, 10, 11,
		12, 13, 14, 12, 14, 15,
		16, 17, 18, 16, 18, 19,
		20, 21, 22, 20, 22, 23,
	};

	result._indices.assign(&i[0], &i[36]);

	for (uint32_t i = 0; i < subDivisions; ++i)
	{
		SubDivide(result);
	}
	return result;
}

GeometryGenerator::MeshData GeometryGenerator::CreateFromTextFile(const std::string& fileName)
{
	MeshData result;
	const std::string fileNameFull = "Models/" + fileName;
	std::ifstream file(fileNameFull.c_str());
	if (!file.is_open())
	{
		ThrowErrCode(ErrCode::PathNotFound, "파일 오픈 실패 !" + fileNameFull);
	}
	std::string line;

	std::getline(file, line);
	const int vertexCount = std::stoi(line.substr(sizeof("VertexCount:")));
	result._vertices.resize(vertexCount);

	std::getline(file, line);
	const int triangleCount = std::stoi(line.substr(sizeof("TriangleCount:")));
	result._indices.resize(3 * triangleCount);

	// 두줄 무시
	std::getline(file, line);
	std::getline(file, line);

	for (int i = 0; i < vertexCount; ++i)
	{
		std::getline(file, line, ' ');
		const float px = std::stod(line);
		std::getline(file, line, ' ');
		const float py = std::stod(line);
		std::getline(file, line, ' ');
		const float pz = std::stod(line);
		std::getline(file, line, ' ');
		const float nx = std::stod(line);
		std::getline(file, line, ' ');
		const float ny = std::stod(line);
		std::getline(file, line, '\n');
		const float nz = std::stod(line);

		result._vertices[i] = VertexData(px, py, pz, nx, ny, nz, 0.f, 0.f, 0.f, 0.f, 0.f);
	}
	// 세줄 무시
	std::getline(file, line);
	std::getline(file, line);
	std::getline(file, line);

	for (int i = 0; i < triangleCount * 3; i += 3)
	{
		std::getline(file, line, ' ');
		result._indices[i] = std::stoi(line);
		std::getline(file, line, ' ');
		result._indices[i + 1] = std::stoi(line);
		std::getline(file, line, '\n');
		result._indices[i + 2] = std::stoi(line);
	}
	file.close();

	return result;
}

void GeometryGenerator::SubDivide(MeshData& result)
{
	check(result._indices.size() % 3 == 0, "index가 비정상입니다.");
	check(result._vertices.size() * 2 < std::numeric_limits<uint16_t>::max(), "index가 너무 커집니다.");

	const MeshData inputCopy = result;
	result._vertices.clear();
	result._indices.clear();
	result._vertices.reserve(result._vertices.size() * 2);
	result._indices.reserve(result._indices.size() * 2);

	const uint32_t baseVertexIndex = result._vertices.size();
	for (int i = 0; i < inputCopy._indices.size(); i += 3)
	{
		const VertexData& v1 = inputCopy._vertices[inputCopy._indices[i]];
		const VertexData& v2 = inputCopy._vertices[inputCopy._indices[i + 1]];
		const VertexData& v3 = inputCopy._vertices[inputCopy._indices[i + 2]];

		VertexData v12 = MidPoint(v1, v2);
		VertexData v23 = MidPoint(v2, v3);
		VertexData v31 = MidPoint(v3, v1);

		result._vertices.push_back(v1);
		result._vertices.push_back(v2);
		result._vertices.push_back(v3);
		result._vertices.push_back(v12);
		result._vertices.push_back(v23);
		result._vertices.push_back(v31);
		
		result._indices.push_back(2 * i + 0);
		result._indices.push_back(2 * i + 3);
		result._indices.push_back(2 * i + 5);

		result._indices.push_back(2 * i + 5);
		result._indices.push_back(2 * i + 3);
		result._indices.push_back(2 * i + 4);

		result._indices.push_back(2 * i + 5);
		result._indices.push_back(2 * i + 4);
		result._indices.push_back(2 * i + 2);

		result._indices.push_back(2 * i + 3);
		result._indices.push_back(2 * i + 1);
		result._indices.push_back(2 * i + 4);
	}
}

VertexData GeometryGenerator::MidPoint(const VertexData& lhs, const VertexData& rhs)
{
	XMVECTOR p0 = XMLoadFloat3(&lhs._position);
	XMVECTOR p1 = XMLoadFloat3(&rhs._position);

	XMVECTOR n0 = XMLoadFloat3(&lhs._normal);
	XMVECTOR n1 = XMLoadFloat3(&rhs._normal);

	XMVECTOR t0 = XMLoadFloat3(&lhs._tangentU);
	XMVECTOR t1 = XMLoadFloat3(&rhs._tangentU);

	XMVECTOR tex0 = XMLoadFloat2(&lhs._textureUV);
	XMVECTOR tex1 = XMLoadFloat2(&rhs._textureUV);

	XMVECTOR pos = 0.5f * (p0 + p1);
	XMVECTOR normal = XMVector3Normalize(0.5f * (n0 + n1));
	XMVECTOR tangent = XMVector3Normalize(0.5f * (t0 + t1));
	XMVECTOR textureUV = 0.5f * (tex0 + tex1);

	VertexData rv;
	XMStoreFloat3(&rv._position, pos);
	XMStoreFloat3(&rv._normal, normal);
	XMStoreFloat3(&rv._tangentU, tangent);
	XMStoreFloat2(&rv._textureUV, textureUV);

	return rv;
}
