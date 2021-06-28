#pragma once
#include "TypeD3d.h"
#include "TypeCommon.h"
#include <unordered_set>
#include "TypeGeometry.h"

class StageInfo;
enum class ErrCode : uint32_t;
class Actor;
class PlayerActor;
class ActionChart;
struct GameObject;
struct GravityPoint;
class MeshGeometry;
struct TerrainObjectInfo;

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
			uint16_t _index;
			//uint8_t _geometryIndex;
		} _leaf;
	};
	DataType _data;
};

struct Terrain
{
public:
	bool isGround(void) const noexcept;
	bool isWall(void) const noexcept;
	Terrain(const TerrainObjectInfo& terrainInfo);
	
private:
	enum class DivideType
	{
		X,
		Y,
		Z,
	};
	GameObject* _gameObject;
	std::vector<TerrainAABBNode> _aabbNodes;
	const MeshGeometry* _mesh;
	const Vertex* _vertexBuffer;
	const GeoIndex* _indexBuffer;

	DirectX::XMFLOAT3 _min;
	DirectX::XMFLOAT3 _max;
	bool _isGround;
	bool _isWall;
private:
	void makeAABBTree(void);
	uint16_t XM_CALLCONV makeAABBTreeImpl(std::vector<int>& terrainIndexList,
									int begin,
									int end,
									DirectX::FXMVECTOR min, 
									DirectX::FXMVECTOR max);

	void sortIndexList(std::vector<int>& terrainIndexList,
					int begin,
					int end,
					DivideType divideType) const noexcept;

	inline float sortCompareValue(int index, DivideType type) const noexcept;

	TerrainAABBNode::DataType::Node XM_CALLCONV getAABBRange(std::vector<int>& terrainIndexList,
															int begin,
															int end,
															DirectX::FXMVECTOR min,
															DirectX::FXMVECTOR max) const noexcept;
};
class StageManager
{
public:
	StageManager();
	~StageManager();
public:
	void loadStage(void);
	void update();
	void moveActorXXX(Actor* actor, const DirectX::XMFLOAT3& moveVector) noexcept;
	bool moveActor(Actor* actor, const TickCount64& deltaTick) noexcept;
	bool rotateActor(Actor* actor, const TickCount64& deltaTick) const noexcept;
	bool applyGravity(Actor* actor, const TickCount64& deltaTick) noexcept;
	bool isLoading(void) const noexcept { return _isLoading; };
	void setNextStage(std::string stageName) noexcept;
	ActionChart* loadActionChartFromXML(const std::string& actionChartName);
	const PlayerActor* getPlayerActor(void) const noexcept;
	const GravityPoint* getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept;
	bool checkCollision(Actor* actor, const DirectX::XMFLOAT3& moveVector) const noexcept;
	bool checkGround(Actor* actor, DirectX::XMFLOAT3& moveVector) const noexcept;
private:
	int sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept;
	std::vector<Terrain> _terrains;
	std::vector<const GameObject*> _terrainObjects;
	DirectX::XMINT3 _sectorSize;
	DirectX::XMINT3 _sectorUnitNumber;
	std::vector<std::unordered_set<Actor*>> _actorsBySector;
	//std::unordered_map<uint32_t, std::vector<uint32_t>> _terrain; // sectorIndex, vector<meshIndexBufferIndex>
	std::vector<std::unique_ptr<Actor>> _actors;
	PlayerActor* _playerActor;
	std::unique_ptr<StageInfo> _stageInfo;
	DirectX::XMINT3 getSectorCoord(const DirectX::XMFLOAT3& position) const noexcept;
	
	std::string _nextStageName;
	bool _isLoading;
	std::unordered_map<std::string, std::unique_ptr<ActionChart>> _actionchartMap;
	void spawnActors();
	void loadStageInfo();
	void updateCamera() noexcept;

	std::string _fixedCameraName;

	int _cameraCount;
	int _cameraIndex;
	void setCameraCount(const int count) noexcept;
	int getCameraCount() const noexcept;
	int getCameraIndex() const noexcept;
	void createMap(void);
};

