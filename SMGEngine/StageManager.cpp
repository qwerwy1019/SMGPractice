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

StageManager::StageManager()
	: _sectorSize(100, 100, 100)
	, _sectorUnitNumber(9, 9, 9)
	, _actorsBySector(9 * 9 * 9)
	, _playerActor(nullptr)
	, _nextStageName("stage00")
	, _isLoading(false)
	, _cameraCount(0)
	, _cameraIndex(-1)
{
}

StageManager::~StageManager()
{
	_actorsBySector.clear();
	_actors.clear();
}

void StageManager::loadStage(void)
{	
	//SMGFramework::getD3DApp()->releaseItemsForStageLoad();
	SMGFramework::getD3DApp()->prepareCommandQueue();
	loadStageInfo();
	createMap();
	spawnActors();
#if defined DEBUG | defined _DEBUG
	for (const auto& actor : _actors)
	{
		SMGFramework::getD3DApp()->createGameObjectDev(actor.get());
	}
#endif
	SMGFramework::getD3DApp()->executeCommandQueue();
	_isLoading = false;
}

void StageManager::update()
{
	TickCount64 deltaTick = SMGFramework::Get().getTimer().getDeltaTickCount();
	
	updateCamera();
	//updateStageScript();

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
	setCulled();
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
	float rotateAngle = actor->getRotateAngleDelta(deltaTick);
	if (MathHelper::equal(rotateAngle, 0.f))
	{
		return false;
	}
	// 회전시 충돌을 고려하는 부분은 나중에 필요하면[5/12/2021 qwerwy]

	actor->rotateOnPlane(rotateAngle);
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

	float t = checkGround(actor, moveVector);
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
// 	if (checkCollision(actor, moveVector))
// 	{
// 		//return true;
// 	}

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

	auto it = _actionchartMap.emplace(actionChartName, new ActionChart(xmlActionChart.getRootNode()));
	check(it.second == true);
	return it.first->second.get();
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
	float t = checkWall(actor, moveVector);
	if (MathHelper::equal(t, 0))
	{
		return true;
	}
	moveVector = MathHelper::mul(moveVector, t);

// 	if (checkCollision(actor, moveVector))
// 	{
// 		//return false;
// 	}

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
}

int StageManager::sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept
{
	return sectorCoord.x * _sectorUnitNumber.y * _sectorUnitNumber.z + sectorCoord.y * _sectorUnitNumber.z + sectorCoord.z;
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

	check(0 <= rv.x && rv.x < _sectorUnitNumber.x, "sector 범위가 작게 설정되지 않았는지 확인해야함");
	check(0 <= rv.y && rv.y < _sectorUnitNumber.y, "sector 범위가 작게 설정되지 않았는지 확인해야함");
	check(0 <= rv.z && rv.z < _sectorUnitNumber.z, "sector 범위가 작게 설정되지 않았는지 확인해야함");

	rv.x = std::clamp(rv.x, 0, _sectorUnitNumber.x - 1);
	rv.y = std::clamp(rv.y, 0, _sectorUnitNumber.y - 1);
	rv.z = std::clamp(rv.z, 0, _sectorUnitNumber.z - 1);

	return rv;
}

void StageManager::killActor(Actor* actor) noexcept
{
	_deadActors.insert(actor);
}

void StageManager::spawnActors()
{
	check(_stageInfo != nullptr);
	const auto& spawnInfos = _stageInfo->getSpawnInfos();
	for (const auto& spawnInfo : spawnInfos)
	{
		std::unique_ptr<Actor> actor;
		auto characterInfo = SMGFramework::getCharacterInfoManager()->getInfo(spawnInfo._key);
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
				check(false, "spawnInfo" + std::to_string(spawnInfo._key) + " characterType이 이상합니다.");
			}
		}
		
		int sectorIndex = sectorCoordToIndex(actor->getSectorCoord());
		check(sectorIndex < _actorsBySector.size());
		auto it = _actorsBySector[sectorIndex].emplace(actor.get());

		_actors.emplace_back(std::move(actor));
	}
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

void StageManager::updateCamera() noexcept
{
	check(_stageInfo != nullptr);
	check(_playerActor != nullptr);

	if (!_fixedCameraName.empty())
	{
		setCameraCount(0);
		const auto& camera = _stageInfo->getFixedCameraPoint(_fixedCameraName);
		SMGFramework::getD3DApp()->setCameraInput(camera._position, camera._focusPosition, camera._upVector, camera._cameraSpeed, camera._cameraFocusSpeed);
	}
	else
	{
		const auto& nearCameraList = _stageInfo->getNearCameraPoints(_playerActor->getPosition());
		setCameraCount(nearCameraList.size());

		if (nearCameraList.empty())
		{
			using namespace MathHelper;
			auto playerPosition = _playerActor->getPosition();
			auto playerDirection = _playerActor->getDirection();
			auto playerUpVector = _playerActor->getUpVector();

			auto cameraFocusPosition = add(playerPosition, mul(playerUpVector, 100));
			auto cameraPosition = add(playerPosition, mul(sub(mul(playerUpVector, 2.f), playerDirection), 600));

			SMGFramework::getD3DApp()->setCameraInput(cameraPosition, cameraFocusPosition, playerUpVector, 20.f, 50.f);
		}
		else
		{
			int cameraIndex = getCameraIndex();
			check(cameraIndex < nearCameraList.size());
			if (cameraIndex == -1)
			{
				using namespace MathHelper;
				float distanceSum = 0.f;
				DirectX::XMFLOAT3 positionSum(0, 0, 0);
				DirectX::XMFLOAT3 upVectorSum(0, 0, 0);
				for (const auto& cam : nearCameraList)
				{
					float distance = length(sub(cam->_position, _playerActor->getPosition()));
					distanceSum += distance;

					positionSum = add(positionSum, mul(cam->_position, distance));
					upVectorSum = add(upVectorSum, mul(cam->_upVector, distance));
				}
				SMGFramework::getD3DApp()->setCameraInput(
					div(positionSum, distanceSum),
					_playerActor->getPosition(),
					div(upVectorSum, distanceSum), 1.f, 1.f);
			}
			else
			{
				SMGFramework::getD3DApp()->setCameraInput(
					nearCameraList[cameraIndex]->_position,
					_playerActor->getPosition(),
					nearCameraList[cameraIndex]->_upVector, 1.f, 1.f);
			}
		}
	}
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
			if (Actor::checkCollision(actor0, actor1))
			{
				actor0->processCollision(actor1);
				actor1->processCollision(actor0);

				auto collisionType0 = actor0->getCharacterInfo()->getCollisionType();
				auto collisionType1 = actor1->getCharacterInfo()->getCollisionType();

				auto moveVector = XMLoadFloat3(&actor0->getPosition()) - XMLoadFloat3(&actor1->getPosition());
				auto moveLength = XMVectorGetX(XMVector3Length(moveVector));
				auto radiusSum = actor0->getRadius() + actor1->getRadius();
				check(moveLength <= radiusSum);
				
				moveVector *= (radiusSum - moveLength) / (moveLength * 10.f);
				
				XMFLOAT3 moveVector0;
				XMStoreFloat3(&moveVector0, moveVector);

				actor0->addMoveVector(moveVector0);
				actor1->addMoveVector(MathHelper::mul(moveVector0, -1));
			}
		}
	}
}

void StageManager::setCameraCount(const int count) noexcept
{
	_cameraCount = count;
	if (_cameraCount <= _cameraIndex)
	{
		_cameraIndex = -1;
	}
}

int StageManager::getCameraCount() const noexcept
{
	return _cameraCount;
}

int StageManager::getCameraIndex() const noexcept
{
	return _cameraIndex;
}

void StageManager::setCulled(void) noexcept
{
	//여기서 절두체 계산을 한번만 하고싶다... 절두체를 그냥 d3d에 박아버리면?
	for (auto& t : _terrains)
	{
		t.setCulled();
	}
	for (auto& actor : _actors)
	{
		actor->setCulled();
	}
}

void StageManager::createMap(void)
{
	_sectorSize = _stageInfo->getSectorSize();
	_sectorUnitNumber = _stageInfo->getSectorUnitNumber();
	_actorsBySector.resize(static_cast<size_t>(_sectorUnitNumber.x) * _sectorUnitNumber.y * _sectorUnitNumber.z);

	const auto& terrainObjectInfos = _stageInfo->getTerrainObjectInfos();
	_terrains.reserve(terrainObjectInfos.size());
	for (const auto& terrainObjectInfo : terrainObjectInfos)
	{
		_terrains.emplace_back(terrainObjectInfo);
	}
}
