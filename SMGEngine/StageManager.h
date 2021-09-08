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
struct GravityPoint;
class Terrain;
class SpawnInfo;
class BackgroundObject;
class StageScript;
class StagePhase;

class StageManager
{
public:
	StageManager();
	~StageManager();
public:
	void update();
	void releaseObjects();
private:
	void processActorCollision(void) noexcept;
	void processActorCollisionXXX(int sectorCoord0, int sectorCoord1) noexcept;
	// 스테이지 이동
public:
	void loadStage(void);
	bool isLoading(void) const noexcept { return _isLoading; };
	void setNextStage(std::string stageName) noexcept;
	ActionChart* loadActionChartFromXML(const std::string& actionChartName);
	void requestSpawn(SpawnInfo&& spawnInfo) noexcept;
	void requestSpawnWithKey(int key) noexcept;
	const StageInfo* getStageInfo(void) const noexcept;
private:
	void spawnStageInfoActors();
	void spawnRequested();
	void spawnActor(const SpawnInfo& spawnInfo);
	void loadStageInfo();
	void loadStageScript(void);
	void createMap(void);
	void unloadStage();

	// 캐릭터 이동
public:
	void moveActorXXX(Actor* actor, const DirectX::XMFLOAT3& moveVector) noexcept;
	bool moveActor(Actor* actor, const TickCount64& deltaTick) noexcept;
	bool rotateActor(Actor* actor, const TickCount64& deltaTick) const noexcept;
	bool applyGravity(Actor* actor, const TickCount64& deltaTick) noexcept;
	float checkWall(Actor* actor, const DirectX::XMFLOAT3& moveVector) const noexcept;
	float checkGround(Actor* actor, const DirectX::XMFLOAT3& moveVector) const noexcept;

	const PlayerActor* getPlayerActor(void) const noexcept;
	const GravityPoint* getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept;

	DirectX::XMINT3 getSectorCoord(const DirectX::XMFLOAT3& position) const noexcept;
	void killActor(Actor* actor) noexcept;

	void setCulled(void) noexcept;

	// stage script
	void addStageScriptVariable(const std::string& name, int value) noexcept;
	int getStageScriptVariable(const std::string& name) const noexcept;
	void updateStageScript(void) noexcept;
private:
	void killActors(void) noexcept;
	int sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept;

private:
	std::vector<Terrain> _terrains;
	std::vector<BackgroundObject> _backgroundObjects;
	DirectX::XMINT3 _sectorSize;
	DirectX::XMINT3 _sectorUnitNumber;
	std::vector<std::unordered_set<Actor*>> _actorsBySector;
	std::unordered_set<Actor*> _deadActors;
	std::vector<SpawnInfo> _requestedSpawnInfos;
	std::vector<int> _requestedSpawnKeys;

	std::unordered_map<std::string, int> _stageScriptVariables;
	StagePhase* _currentPhase;
	std::unique_ptr<StageScript> _stageScript;

	std::vector<std::unique_ptr<Actor>> _actors;
	PlayerActor* _playerActor;
	std::unique_ptr<StageInfo> _stageInfo;
	
	std::string _nextStageName;
	bool _isLoading;
	std::unordered_map<std::string, std::unique_ptr<ActionChart>> _actionchartMap;
	void loadUI();
};

