#pragma once
#include "TypeGeometry.h"

struct TerrainObjectInfo;
class MeshGeometry;
class GameObject;

struct TerrainAABBNode
{
	TerrainAABBNode() = default;
	~TerrainAABBNode() = default;

	std::array<uint16_t, 2> _children;
	union DataType
	{
		struct Node
		{
			uint8_t _minX;
			uint8_t _minY;
			uint8_t _minZ;
			uint8_t _maxX;
			uint8_t _maxY;
			uint8_t _maxZ;
		} _node;
		struct Leaf
		{
			int _index;
			uint8_t _subMeshIndex;
			//uint8_t _geometryIndex;
		} _leaf;
	};
	DataType _data;
};
class Terrain
{
public:
	bool isGround(void) const noexcept;
	bool isWall(void) const noexcept;
	Terrain(const TerrainObjectInfo& terrainInfo);
	float checkCollision(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& moveVector, float size) const noexcept;
	float XM_CALLCONV checkCollisionXXX(int nodeIndex, const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to, DirectX::FXMVECTOR min, DirectX::FXMVECTOR max) const noexcept;

private:
	enum class DivideType
	{
		X,
		Y,
		Z,
	};
	GameObject* _gameObject;
	std::vector<TerrainAABBNode> _aabbNodes;
	const MeshGeometry* _meshGeometry;
	const Vertex* _vertexBuffer;
	const GeoIndex* _indexBuffer;

	DirectX::XMFLOAT3 _min;
	DirectX::XMFLOAT3 _max;
	bool _isGround;
	bool _isWall;
private:
	void makeAABBTree(void);
	uint16_t XM_CALLCONV makeAABBTreeXXX(std::vector<TerrainAABBNode::DataType::Leaf>& terrainIndexList,
		int begin,
		int end,
		DirectX::FXMVECTOR min,
		DirectX::FXMVECTOR max);
	const Vertex& getVertexFromLeafNode(const TerrainAABBNode::DataType::Leaf& leafNode, const int offset) const noexcept;
	void sortIndexList(std::vector<TerrainAABBNode::DataType::Leaf>& terrainIndexList,
		int begin,
		int end,
		DivideType divideType) const noexcept;

	inline float sortCompareValue(const TerrainAABBNode::DataType::Leaf& index, DivideType type) const noexcept;

	TerrainAABBNode::DataType::Node XM_CALLCONV getAABBRange(std::vector<TerrainAABBNode::DataType::Leaf>& terrainIndexList,
		int begin,
		int end,
		DirectX::FXMVECTOR min,
		DirectX::FXMVECTOR max) const noexcept;
};