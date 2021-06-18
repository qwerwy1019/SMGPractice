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

StageManager::StageManager()
	: _sectorSize(100, 100, 100)
	, _sectorUnitNumber(9, 9, 9)
	, _stageInfo(nullptr)
	, _isLoading(false)
	, _nextStageName("stage00")
	, _actorsBySector(9 * 9 * 9)
{
	//loadStage();
}

StageManager::~StageManager()
{
	_actorsBySector.clear();
	_terrainObjects.clear();
	_actors.clear();
}

void StageManager::loadStage(void)
{	
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

bool StageManager::applyGravity(Actor* actor, const TickCount64& deltaTick) const noexcept
{
	return false;
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

bool StageManager::moveActor(Actor* actor, const TickCount64& deltaTick) noexcept
{
	const XMFLOAT3 zeroVector(0, 0, 0);
	XMFLOAT3 moveVector = actor->getMoveVector(deltaTick);
	if (MathHelper::equal(moveVector, zeroVector))
	{
		return false;
	}

	XMFLOAT3 position = actor->getPosition();
	XMINT3 sector = getSectorCoord(actor->getPosition());
	const int sectorIndex = sectorCoordToIndex(sector);
	check(sectorIndex < _actorsBySector.size(), "sector 범위를 벗어납니다.");

	XMVECTOR moveVectorLoaded = XMLoadFloat3(&moveVector);
	for (int i = std::max(0, sector.x - 1); i < std::min(_sectorUnitNumber.x, sector.x + 1); ++i)
	{
		for (int j = std::max(0, sector.y - 1); j < std::min(_sectorUnitNumber.y, sector.y + 1); ++j)
		{
			for (int k = std::max(0, sector.z - 1); k < std::min(_sectorUnitNumber.z, sector.z + 1); ++k)
			{
				const auto& actorsInSector = _actorsBySector[sectorCoordToIndex(XMINT3(i, j, k))];
				for (const auto& otherActor : actorsInSector)
				{
					if (otherActor == actor)
					{
						continue;
					}

					CollisionInfo outCollisionInfo;
					if (actor->checkCollision(otherActor, moveVectorLoaded, outCollisionInfo))
					{
						return false;
					}
				}
			}
		}
	}

	XMFLOAT3 toPosition = MathHelper::add(position, moveVector);
	const int toSectorIndex = sectorCoordToIndex(getSectorCoord(toPosition));
	if (sectorIndex != toSectorIndex)
	{
		_actorsBySector[sectorIndex].erase(actor);
		_actorsBySector[toSectorIndex].insert(actor);
	}

	actor->setPosition(toPosition);
	return true;
}

int StageManager::sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept
{
	return sectorCoord.x * _sectorUnitNumber.x + sectorCoord.y * _sectorUnitNumber.y + sectorCoord.z * _sectorUnitNumber.z;
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
	// todo [5/22/2021 qwerwy]
	check(0 <= rv.x && rv.x < _sectorUnitNumber.x);
	check(0 <= rv.y && rv.y < _sectorUnitNumber.y);
	check(0 <= rv.z && rv.z < _sectorUnitNumber.z);

	return rv;
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
		
		int sectorIndex = sectorCoordToIndex(getSectorCoord(spawnInfo._position));
		check(sectorIndex < _actorsBySector.size());
		_actorsBySector[sectorIndex].emplace(actor.get());
		if (spawnInfo._isTerrain)
		{
			_terrainObjects.push_back(actor->getGameObject());
		}
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
			auto cameraPosition = add(playerPosition, mul(sub(playerUpVector, mul(playerDirection, 2.f)), 500));

			SMGFramework::getD3DApp()->setCameraInput(cameraPosition, cameraFocusPosition, playerUpVector, 10.f, 10.f);
		}
		else
		{
			int cameraIndex = getCameraIndex();
			check(cameraIndex < nearCameraList.size());
			if (cameraIndex != -1)
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

void StageManager::createMap(void)
{
	_sectorSize = _stageInfo->getSectorSize();
	_sectorUnitNumber = _stageInfo->getSectorUnitNumber();
	const auto& terrainObjectInfos = _stageInfo->getTerrainObjectInfos();
	for (const auto& terrainObjectInfo : terrainObjectInfos)
	{
		GameObject* terrainObject = SMGFramework::getD3DApp()->createObjectFromXML(terrainObjectInfo._objectFileName);
		
		MathHelper::getWorldMatrix(terrainObjectInfo._position,
									terrainObjectInfo._direction, 
									terrainObjectInfo._upVector, 
									terrainObjectInfo._size, 
									terrainObject->_worldMatrix);
		terrainObject->_dirtyFrames = FRAME_RESOURCE_COUNT;
#if defined DEBUG | defined _DEBUG
		SMGFramework::getD3DApp()->createGameObjectDev(terrainObject);
#endif
		_terrainObjects.push_back(terrainObject);
	}
}
