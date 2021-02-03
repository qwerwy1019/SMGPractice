#pragma once
#include "stdafx.h"

struct VertexData
{
	VertexData()
		: _position(0.f, 0.f, 0.f)
		, _normal(0.f, 0.f, 0.f)
		, _tangentU(0.f, 0.f, 0.f)
		, _textureUV(0.f, 0.f) {};
	VertexData(const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& normalVector,
		const DirectX::XMFLOAT3& tangentU,
		const DirectX::XMFLOAT2& uv)
		: _position(position)
		, _normal(normalVector)
		, _tangentU(tangentU)
		, _textureUV(uv) {}
	VertexData(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tgx, float tgy, float tgz,
		float tu, float tv)
		: _position(px, py, pz)
		, _normal(nx, ny, nz)
		, _tangentU(tgx, tgy, tgz)
		, _textureUV(tu, tv) {}
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _normal;
	DirectX::XMFLOAT3 _tangentU;
	DirectX::XMFLOAT2 _textureUV;
};

class GeometryGenerator
{
public:
	
	struct MeshData
	{
		std::vector<VertexData> _vertices;
		std::vector<uint16_t> _indices;
	};
	static MeshData CreateCylinder(
		float bottomRadius,
		float topRadius, 
		float height, 
		uint32_t sliceCount, 
		uint32_t stackCount);
	static MeshData CreateSphere(
		float radius,
		uint32_t sliceCount,
		uint32_t stackCount);
	static MeshData CreateGeosphere(
		float radius,
		uint32_t subDivisions);
	static MeshData CreateGrid(
		float width,
		float depth,
		uint32_t m,
		uint32_t n);
	static MeshData CreateBox(
		float width,
		float height,
		float depth,
		uint32_t subDivisions);
	static MeshData CreateFromTextFile(const std::string& fileName);
private:
	static void SubDivide(MeshData& result);
	static VertexData MidPoint(const VertexData& lhs, const VertexData& rhs);
};

