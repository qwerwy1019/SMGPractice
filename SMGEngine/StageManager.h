#pragma once
#include "TypeD3d.h"
#include "TypeCommon.h"
#include <unordered_set>

class StageInfo;
enum class ErrCode : uint32_t;
class Actor;

class StageManager
{
public:
	StageManager();
	~StageManager();
public:
	void loadStage(void);
	void update();
	bool moveActor(Actor* actor, const TickCount64& deltaTick) noexcept;
	bool rotateActor(Actor* actor, const TickCount64& deltaTick) const noexcept;
	bool applyGravity(Actor* actor, const TickCount64& deltaTick) const noexcept;
	bool isLoading(void) const noexcept { return _isLoading; };
	void setNextStage(std::string stageName) noexcept;
private:
	int sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept;
	std::string _terrainMeshName;
	DirectX::XMINT3 _sectorSize;
	DirectX::XMINT3 _sectorUnitNumber;
	std::vector<std::unordered_set<Actor*>> _actorsBySector;
	std::unordered_map<uint32_t, std::vector<uint32_t>> _terrain; // sectorIndex, vector<meshIndexBufferIndex>
	std::vector<std::unique_ptr<Actor>> _actors;
	std::unique_ptr<StageInfo> _stageInfo;
	DirectX::XMINT3 getSectorCoord(const DirectX::XMFLOAT3& position) const noexcept;
	
	std::string _nextStageName;
	bool _isLoading;
	void spawnActors();
};

