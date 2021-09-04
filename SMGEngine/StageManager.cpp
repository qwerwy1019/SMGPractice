#include "stdafx.h"
#include "StageManager.h"
#include "StageInfo.h"
#include "D3DApp.h"
#include "SMGFramework.h"
#include "Actor.h"
#include "MathHelper.h"
#include "FileHelper.h"
#include "CharacterInfoManager.h"
#include "ActionChart.h"
#include "FrameEvent.h"
#include "ActionCondition.h"
#include <algorithm>
#include "MeshGeometry.h"
#include "Terrain.h"
#include "BackgroundObject.h"
#include "ObjectInfo.h"
#include "StageScript.h"

StageManager::StageManager()
	: _sectorSize(100, 100, 100)
	, _sectorUnitNumber(9, 9, 9)
	, _actorsBySector(9 * 9 * 9)
	, _currentPhase(nullptr)
	, _playerActor(nullptr)
	, _nextStageName("stage00")
	, _isLoading(false)
{
}

StageManager::~StageManager()
{
}

void StageManager::loadStage(void)
{	
	unloadStage();

	SMGFramework::getD3DApp()->prepareCommandQueue();
	loadStageInfo();
	loadStageScript();
	createMap();
	spawnStageInfoActors();

	SMGFramework::getD3DApp()->executeCommandQueue();

	SMGFramework::getD3DApp()->setLight(_stageInfo->getLights(), _stageInfo->getAmbientLight());
	_isLoading = false;
}

void StageManager::update()
{
	TickCount64 deltaTick = SMGFramework::Get().getTimer().getDeltaTickCount();
	

	for (const auto& actor : _actors)
	{
		actor->updateActor(deltaTick);

		bool isChanged = false;
		isChanged |= rotateActor(actor.get(), deltaTick);
		isChanged |= moveActor(actor.get(), deltaTick);
		isChanged |= applyGravity(actor.get(), deltaTick);

		if (isChanged)
		{
			actor->updateObjectWorldMatrix();
		}
	}
	processActorCollision();
	killActors();
	updateStageScript();
	spawnRequested();
}

void StageManager::releaseObjects()
{
	// d3dapp relaseItemsForStageLoad �۾� �Ϸ�� �����Ұ� [9/3/2021 qwerw]
	//_isLoading = true;
	unloadStage();
}

void StageManager::moveActorXXX(Actor* actor, const DirectX::XMFLOAT3& moveVector) noexcept
{
	XMFLOAT3 position = actor->getPosition();
	XMFLOAT3 toPosition = MathHelper::add(position, moveVector);
	const int sectorIndex = sectorCoordToIndex(actor->getSectorCoord());

	actor->setPosition(toPosition);
	const int toSectorIndex = sectorCoordToIndex(actor->getSectorCoord());

	if (sectorIndex != toSectorIndex)
	{
		_actorsBySector[sectorIndex].erase(actor);
		_actorsBySector[toSectorIndex].insert(actor);
	}
}

bool StageManager::rotateActor(Actor* actor, const TickCount64& deltaTick) const noexcept
{
	if (actor->isQuaternionRotate())
	{
		DirectX::XMFLOAT4 rotationQuat = actor->getRotationQuat(deltaTick);

		actor->rotateQuat(rotationQuat);
	}
	else
	{
		float rotateAngle = actor->getRotateAngleDelta(deltaTick);
		if (MathHelper::equal(rotateAngle, 0.f))
		{
			return false;
		}
		// ȸ���� �浹�� ����ϴ� �κ��� ���߿� �ʿ��ϸ�[5/12/2021 qwerwy]

		actor->rotateOnPlane(rotateAngle);
	}

	return true;
}

bool StageManager::applyGravity(Actor* actor, const TickCount64& deltaTick) noexcept
{
	XMFLOAT3 gravityDirection;
	bool isGravityApplied = actor->getGravityDirection(gravityDirection);
	if (false == isGravityApplied)
	{
		return false;
	}

	actor->applyGravityRotation(deltaTick, gravityDirection);
	

	float speed = actor->getVerticalSpeed();
	if (MathHelper::equal(speed, 0))
	{
		return true;
	}
	XMFLOAT3 moveVector = MathHelper::mul(gravityDirection, speed);

	float t = actor->isCollisionOn() ? checkGround(actor, moveVector) : 1.f;
	if (MathHelper::equal(t, 0))
	{
		return true;
	}

	if (t < 1.f)
	{
		actor->setActorOnGround(true);
	}
	else
	{
		actor->setActorOnGround(false);
	}

	moveVector = MathHelper::mul(moveVector, t);

	moveActorXXX(actor, moveVector);

	return true;
}

void StageManager::setNextStage(std::string stageName) noexcept
{
	_nextStageName = stageName;
	_isLoading = true;
}

ActionChart* StageManager::loadActionChartFromXML(const std::string& actionChartName)
{
	auto findIt = _actionchartMap.find(actionChartName);
	if (findIt != _actionchartMap.end())
	{
		return findIt->second.get();
	}

	const std::string filePath = "../Resources/XmlFiles/ActionChart/" + actionChartName + ".xml";
	XMLReader xmlActionChart;
	xmlActionChart.loadXMLFile(filePath);

	auto it = _actionchartMap.emplace(actionChartName, std::make_unique<ActionChart>(xmlActionChart.getRootNode()));
	if (it.second == false)
	{
		ThrowErrCode(ErrCode::KeyDuplicated, actionChartName);
	}
	return it.first->second.get();
}

void StageManager::requestSpawn(SpawnInfo&& spawnInfo) noexcept
{
	_requestedSpawnInfos.emplace_back(spawnInfo);
}

void StageManager::requestSpawnWithKey(int key) noexcept
{
	_requestedSpawnKeys.emplace_back(key);
}

const StageInfo* StageManager::getStageInfo(void) const noexcept
{
	return _stageInfo.get();
}

void StageManager::spawnActor(const SpawnInfo& spawnInfo)
{
	std::unique_ptr<Actor> actor;
	auto characterInfo = SMGFramework::getCharacterInfoManager()->getInfo(spawnInfo.getCharacterKey());
	check(characterInfo != nullptr);
	switch (characterInfo->getCharacterType())
	{
		case CharacterType::Player:
		{
			actor = std::make_unique<PlayerActor>(spawnInfo);
			_playerActor = static_cast<PlayerActor*>(actor.get());
		}
		break;
		case CharacterType::Monster:
		case CharacterType::Object:
		{
			actor = std::make_unique<Actor>(spawnInfo);
		}
		break;
		case CharacterType::Count:
		default:
		{
			static_assert(static_cast<int>(CharacterType::Count) == 3);
			check(false, "spawnInfo" + std::to_string(spawnInfo.getCharacterKey()) + " characterType�� �̻��մϴ�.");
		}
	}

	int sectorIndex = sectorCoordToIndex(actor->getSectorCoord());
	check(sectorIndex < _actorsBySector.size());
	auto it = _actorsBySector[sectorIndex].emplace(actor.get());

#if defined DEBUG | defined _DEBUG
	SMGFramework::getD3DApp()->createGameObjectDev(actor.get());
#endif
	_actors.emplace_back(std::move(actor));
}

const PlayerActor* StageManager::getPlayerActor(void) const noexcept
{
	return _playerActor;
}

const GravityPoint* StageManager::getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept
{
	if (_stageInfo == nullptr)
	{
		check(false);
		return nullptr;
	}

	return _stageInfo->getGravityPointAt(position);
}

float StageManager::checkWall(Actor* actor, const DirectX::XMFLOAT3& moveVector) const noexcept
{
	float minCollisionTime = 1.f;
	for (const auto& terrain : _terrains)
	{
		if (terrain.isWall() == false)
		{
			continue;
		}
		float collisionTime;
		if (terrain.checkCollision(*actor, moveVector, collisionTime) && collisionTime < minCollisionTime)
		{
			minCollisionTime = collisionTime;
		}
	}
	return minCollisionTime;
}
float StageManager::checkGround(Actor* actor, const DirectX::XMFLOAT3& moveVector) const noexcept
{
	float minCollisionTime = 1.f;
	for (const auto& terrain : _terrains)
	{
		if (terrain.isGround() == false)
		{
			continue;
		}
		float collisionTime;
		if (terrain.checkCollision(*actor, moveVector, collisionTime) && collisionTime < minCollisionTime)
		{
			minCollisionTime = collisionTime;
		}
	}
	return minCollisionTime;
}

bool StageManager::moveActor(Actor* actor, const TickCount64& deltaTick) noexcept
{
	const XMFLOAT3 zeroVector(0, 0, 0);
	XMFLOAT3 moveVector = actor->getMoveVector(deltaTick);
	if (MathHelper::equal(moveVector, zeroVector))
	{
		return false;
	}
	float t = actor->isCollisionOn() ? checkWall(actor, moveVector) : 1.f;
	if (MathHelper::equal(t, 0))
	{
		return true;
	}
	moveVector = MathHelper::mul(moveVector, t);

	moveActorXXX(actor, moveVector);
	return true;
}

void StageManager::killActors(void) noexcept
{
	for (auto actor : _deadActors)
	{
		int sectorIndex = sectorCoordToIndex(actor->getSectorCoord());
		size_t erased = _actorsBySector[sectorIndex].erase(actor);
		check(erased == 1);
		
		for (int i = 0; i < _actors.size(); ++i)
		{
			if (_actors[i].get() == actor)
			{
				_actors[i] = std::move(_actors.back());
				_actors.pop_back();
				break;
			}
		}
	}
	_deadActors.clear();
}

int StageManager::sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept
{
	return sectorCoord.x * _sectorUnitNumber.y * _sectorUnitNumber.z + sectorCoord.y * _sectorUnitNumber.z + sectorCoord.z;
}

void StageManager::loadStageScript(void)
{
	check(_stageInfo != nullptr);
	
	std::string stageScriptFilePath =
		"../Resources/XmlFiles/StageScript/" +
		_stageInfo->getStageScriptName() + ".xml";

	XMLReader xmlStageScript;

	xmlStageScript.loadXMLFile(stageScriptFilePath);

	_stageScript = std::make_unique<StageScript>(xmlStageScript.getRootNode());

	_currentPhase = _stageScript->getStartPhase();
	_stageScriptVariables = _stageScript->getVariables();

}

DirectX::XMINT3 StageManager::getSectorCoord(const DirectX::XMFLOAT3& position) const noexcept
{
	check(_sectorSize.x != 0);
	check(_sectorSize.y != 0);
	check(_sectorSize.z != 0);

	DirectX::XMFLOAT3 baseCoord = { static_cast<float>((_sectorUnitNumber.x / 2) * _sectorSize.x),
									static_cast<float>((_sectorUnitNumber.y / 2) * _sectorSize.y),
									static_cast<float>((_sectorUnitNumber.z / 2) * _sectorSize.z) };

	DirectX::XMINT3 rv = { static_cast<int>(position.x + baseCoord.x) / _sectorSize.x,
						   static_cast<int>(position.y + baseCoord.y) / _sectorSize.y,
						   static_cast<int>(position.z + baseCoord.z) / _sectorSize.z };

	check(0 <= rv.x && rv.x < _sectorUnitNumber.x, "sector ������ �۰� �������� �ʾҴ��� Ȯ���ؾ���");
	check(0 <= rv.y && rv.y < _sectorUnitNumber.y, "sector ������ �۰� �������� �ʾҴ��� Ȯ���ؾ���");
	check(0 <= rv.z && rv.z < _sectorUnitNumber.z, "sector ������ �۰� �������� �ʾҴ��� Ȯ���ؾ���");

	rv.x = std::clamp(rv.x, 0, _sectorUnitNumber.x - 1);
	rv.y = std::clamp(rv.y, 0, _sectorUnitNumber.y - 1);
	rv.z = std::clamp(rv.z, 0, _sectorUnitNumber.z - 1);

	return rv;
}

void StageManager::killActor(Actor* actor) noexcept
{
	_deadActors.insert(actor);
}

void StageManager::spawnStageInfoActors()
{
	check(_stageInfo != nullptr);
	const auto& spawnInfos = _stageInfo->getSpawnInfos();
	for (const auto& spawnInfo : spawnInfos)
	{
		spawnActor(spawnInfo);
	}
}

void StageManager::spawnRequested()
{
	if (_requestedSpawnInfos.empty() && _requestedSpawnKeys.empty())
	{
		return;
	}

	SMGFramework::getD3DApp()->prepareCommandQueue();
	for (const auto& spawnInfo : _requestedSpawnInfos)
	{
		spawnActor(spawnInfo);
	}
	_requestedSpawnInfos.clear();

	for (const auto& spawnKey : _requestedSpawnKeys)
	{
		spawnActor(_stageInfo->getSpawnInfoWithKey(spawnKey));
	}
	_requestedSpawnKeys.clear();

	SMGFramework::getD3DApp()->executeCommandQueue();
}

void StageManager::loadStageInfo()
{
	_stageInfo = std::make_unique<StageInfo>();

	const std::string stageInfoFilePath =
		"../Resources/XmlFiles/StageInfo/" + _nextStageName + ".xml";
	XMLReader xmlStageInfo;

	xmlStageInfo.loadXMLFile(stageInfoFilePath);

	_stageInfo->loadXml(xmlStageInfo.getRootNode());
}

void StageManager::processActorCollision(void) noexcept
{
	for (int i = 0; i < _sectorUnitNumber.x - 1; ++i)
	{
		for (int j = 0; j < _sectorUnitNumber.y - 1; ++j)
		{
			for (int k = 0; k < _sectorUnitNumber.z - 1; ++k)
			{
				auto sector0Index = sectorCoordToIndex(XMINT3(i, j, k));
				if (_actorsBySector[sector0Index].empty())
				{
					continue;
				}
				for (int ii = 0; ii < 2; ++ii)
				{
					for (int jj = 0; jj < 2; ++jj)
					{
						for (int kk = 0; kk < 2; ++kk)
						{
							processActorCollisionXXX(sector0Index, sectorCoordToIndex(XMINT3(i+ii, j+jj, k+kk)));
						}
					}
				}
			}
		}
	}
}

void StageManager::processActorCollisionXXX(int sectorCoord0, int sectorCoord1) noexcept
{
	check(sectorCoord0 < _actorsBySector.size());
	check(sectorCoord1 < _actorsBySector.size());

	const auto& actorsInSector0 = _actorsBySector[sectorCoord0];
	const auto& actorsInSector1 = _actorsBySector[sectorCoord1];

	for (const auto& actor0 : actorsInSector0)
	{
		for (const auto& actor1 : actorsInSector1)
		{
			if (actor0 == actor1)
			{
				continue;
			}
			if (sectorCoord0 == sectorCoord1 && actor1 < actor0)
			{
				continue;
			}
			if (!actor0->isCollisionOn() || !actor1->isCollisionOn())
			{
				continue;
			}
			if (Actor::checkCollision(actor0, actor1))
			{
				CollisionCase actor0Case = CollisionCase::Center;
				CollisionCase actor1Case = CollisionCase::Center;

				XMVECTOR position0 = XMLoadFloat3(&actor0->getPosition());
				XMVECTOR position1 = XMLoadFloat3(&actor1->getPosition());
				XMVECTOR upVector0 = XMLoadFloat3(&actor0->getUpVector());
				XMVECTOR upVector1 = XMLoadFloat3(&actor1->getUpVector());

				float actor0HeightFromActor1 = XMVectorGetX(XMVector3Dot(position0 - position1, upVector0));
				float actor1HeightFromActor0 = XMVectorGetX(XMVector3Dot(position1 - position0, upVector1));
				
				if (std::abs(actor0HeightFromActor1) > actor0->getHalfHeight() &&
					std::abs(actor1HeightFromActor0) > actor1->getHalfHeight())
				{
					check((actor0HeightFromActor1 < 0) != (actor1HeightFromActor0 < 0), 
						"�ݴ�� ���ִ� ���͵鳢�� ���� �浹�Ѵٸ� ������ �ʿ���.");
					if (actor0HeightFromActor1 < 0)
					{
						actor0Case = CollisionCase::Lower;
						actor1Case = CollisionCase::Upper;
					}
					else
					{
						actor0Case = CollisionCase::Upper;
						actor1Case = CollisionCase::Lower;
					}
				}
				actor0->processCollision(actor1, actor0Case);
				actor1->processCollision(actor0, actor1Case);

				auto moveVector = XMLoadFloat3(&actor0->getPosition()) - XMLoadFloat3(&actor1->getPosition());
				auto moveLength = XMVectorGetX(XMVector3Length(moveVector));
				auto radiusSum = actor0->getRadius() + actor1->getRadius();
				check(moveLength <= radiusSum);
				if (MathHelper::equal(moveLength, 0.f))
				{
					moveVector = XMLoadFloat3(&actor0->getDirection()) * -radiusSum;
				}
				else
				{
					moveVector *= (radiusSum - moveLength) / (moveLength);
				}
				
				XMFLOAT3 moveVector0;
				XMStoreFloat3(&moveVector0, moveVector);

				actor0->addMoveVector(MathHelper::mul(moveVector0, Actor::getResistanceDistance(*actor0, *actor1)));
				actor1->addMoveVector(MathHelper::mul(moveVector0, -Actor::getResistanceDistance(*actor1, *actor0)));
			}
		}
	}
}

void StageManager::updateStageScript(void) noexcept
{
	check(_currentPhase != nullptr);

	_currentPhase->processFunctions();
	std::string nextPhaseName;
	bool changePhase = _currentPhase->getNextPhaseName(nextPhaseName);
	if (changePhase)
	{
		_currentPhase = _stageScript->getPhase(nextPhaseName);
	}
}

void StageManager::setCulled(void) noexcept
{
	for (auto& backgroundObject : _backgroundObjects)
	{
		backgroundObject.setCulled();
	}
	for (auto& t : _terrains)
	{
		t.setCulled();
	}
	for (auto& actor : _actors)
	{
		actor->setCulled();
	}
}

void StageManager::addStageScriptVariable(const std::string& name, int value) noexcept
{
	auto it = _stageScriptVariables.find(name);
	if (it == _stageScriptVariables.end())
	{
		check(false, name);
		return;
	}
	it->second += value;
}

int StageManager::getStageScriptVariable(const std::string& name) const noexcept
{
	auto it = _stageScriptVariables.find(name);
	if (it == _stageScriptVariables.end())
	{
		check(false, name);
		return 0;
	}
	return it->second;
}

void StageManager::createMap(void)
{
	_sectorSize = _stageInfo->getSectorSize();
	_sectorUnitNumber = _stageInfo->getSectorUnitNumber();
	_actorsBySector.resize(static_cast<size_t>(_sectorUnitNumber.x) * _sectorUnitNumber.y * _sectorUnitNumber.z);

	SMGFramework::getD3DApp()->setBackgroundColor(_stageInfo->getBackgroundColor());

	const auto& backgroundObjectInfos = _stageInfo->getBackgroundObjectInfos();
	_backgroundObjects.reserve(backgroundObjectInfos.size());
	for (const auto& backgroundObjectInfo : backgroundObjectInfos)
	{
		_backgroundObjects.emplace_back(backgroundObjectInfo);
	}

	const auto& terrainObjectInfos = _stageInfo->getTerrainObjectInfos();
	_terrains.reserve(terrainObjectInfos.size());
	for (const auto& terrainObjectInfo : terrainObjectInfos)
	{
		_terrains.emplace_back(terrainObjectInfo);
	}
	
	SMGFramework::getD3DApp()->createEffectMeshGeometry();
	const auto& effectFileNames = _stageInfo->getEffectFileNames();
	for (const auto& effectFileName : effectFileNames)
	{
		SMGFramework::getD3DApp()->loadXMLEffectFile(effectFileName);
	}
}

void StageManager::unloadStage()
{
	_terrains.clear();
	_backgroundObjects.clear();
	_requestedSpawnInfos.clear();
	_requestedSpawnKeys.clear();
	_stageScriptVariables.clear();
	_stageScript = nullptr;
	_actorsBySector.clear();
	_deadActors.clear();
	_actors.clear();
	_playerActor = nullptr;

	_stageInfo = nullptr;
	_actionchartMap.clear();
	
	SMGFramework::getD3DApp()->releaseItemsForStageLoad();
}
