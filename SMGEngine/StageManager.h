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

class StageManager
{
public:
	StageManager();
	~StageManager();
public:
	void update();
private:
	void processActorCollision(void) noexcept;
	void processActorCollisionXXX(int sectorCoord0, int sectorCoord1) noexcept;
	void updateStageScript(void) noexcept;
	// 스테이지 이동
public:
	void loadStage(void);
	bool isLoading(void) const noexcept { return _isLoading; };
	void setNextStage(std::string stageName) noexcept;
	ActionChart* loadActionChartFromXML(const std::string& actionChartName);
	void requestSpawn(SpawnInfo&& spawnInfo) noexcept;
	const StageInfo* getStageInfo(void) const noexcept;
private:
	void spawnStageInfoActors();
	void spawnRequested();
	void spawnActor(const SpawnInfo& spawnInfo);
	void loadStageInfo();
	void createMap(void);

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
private:
	void killActors(void) noexcept;
	int sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept;

	//카메라
private:
	void setCameraCount(const int count) noexcept;
	int getCameraCount() const noexcept;
	int getCameraIndex() const noexcept;

private:
	std::vector<Terrain> _terrains;
	std::vector<BackgroundObject> _backgroundObjects;
	DirectX::XMINT3 _sectorSize;
	DirectX::XMINT3 _sectorUnitNumber;
	std::vector<std::unordered_set<Actor*>> _actorsBySector;
	std::unordered_set<Actor*> _deadActors;
	std::vector<SpawnInfo> _requestedSpawnInfos;
	std::vector<std::string, int> _stageScriptVariables;

	std::vector<std::unique_ptr<Actor>> _actors;
	PlayerActor* _playerActor;
	std::unique_ptr<StageInfo> _stageInfo;
	
	std::string _nextStageName;
	bool _isLoading;
	std::unordered_map<std::string, std::unique_ptr<ActionChart>> _actionchartMap;

	std::string _fixedCameraName;

	int _cameraCount;
	int _cameraIndex;
};

