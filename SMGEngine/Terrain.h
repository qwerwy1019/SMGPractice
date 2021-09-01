#pragma once
#include "TypeGeometry.h"
#include "TypeAction.h"

class TerrainObjectInfo;
class ObjectInfo;
class MeshGeometry;
class GameObject;
class Actor;

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

// 스택 공간에서만 사용한다면 괜찮을지도? [7/12/2021 qwerw]
struct TerrainCollisionInfoXXX
{
	CollisionShape _shape;
	DirectX::XMVECTOR _position;
	DirectX::XMVECTOR _velocity;
	DirectX::XMVECTOR _boxX;
	DirectX::XMVECTOR _boxY;
	DirectX::XMVECTOR _boxZ;
	DirectX::XMVECTOR _min;
	DirectX::XMVECTOR _max;
	float _radius;
};

class Terrain
{
public:
	bool isGround(void) const noexcept;
	bool isWall(void) const noexcept;
	Terrain(const TerrainObjectInfo& terrainInfo);
	~Terrain();
	bool checkCollision(const Actor& actor, const DirectX::XMFLOAT3& velocity, float& collisionTime) const noexcept;
	float XM_CALLCONV checkCollisionXXX(int nodeIndex, 
										DirectX::FXMVECTOR min,
										DirectX::FXMVECTOR max, 
										const TerrainCollisionInfoXXX& collisionInfo) const noexcept;
	void setCulled(void) noexcept;
private:
	enum class DivideType
	{
		X,
		Y,
		Z,
	};
	GameObject* _gameObject;
	std::vector<TerrainAABBNode> _aabbNodes;
	const Vertex* _vertexBuffer;
	const GeoIndex* _indexBuffer;

	DirectX::XMFLOAT3 _min;
	DirectX::XMFLOAT3 _max;
	bool _isGround;
	bool _isWall;
	float _size;
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