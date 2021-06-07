#pragma once
#include "stdafx.h"
#include "TypeGeometry.h"

class GeometryGenerator
{
public:
	static GeneratedMeshData CreateCylinder(
		float bottomRadius,
		float topRadius, 
		float height, 
		uint32_t sliceCount, 
		uint32_t stackCount);
	static GeneratedMeshData CreateSphere(
		float radius,
		uint32_t sliceCount,
		uint32_t stackCount);
	static GeneratedMeshData CreateGeosphere(
		float radius,
		uint32_t subDivisions);
	static GeneratedMeshData CreateGrid(
		float width,
		float depth,
		uint32_t m,
		uint32_t n);
	static GeneratedMeshData CreateBox(
		float width,
		float height,
		float depth,
		uint32_t subDivisions);
	static GeneratedMeshData CreateFromTextFile(const std::string& fileName);
private:
	static void SubDivide(GeneratedMeshData& result);
	static Vertex MidPoint(const Vertex& lhs, const Vertex& rhs);
};

