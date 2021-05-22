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
	: _sectorSize(0, 0, 0)
	, _sectorUnitNumber(0, 0, 0)
	, _stageInfo(nullptr)
	, _isLoading(false)
{

}

StageManager::~StageManager()
{
	_actorsBySector.clear();
	_terrain.clear();
	_actors.clear();
}

void StageManager::loadStage(void)
{	
	loadStageInfo();
	//createMap();
	spawnActors();
	_isLoading = false;
}

void StageManager::update()
{
	TickCount64 deltaTick = SMGFramework::Get().getTimer().getDeltaTickCount();

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
	// ȸ���� �浹�� ����ϴ� �κ��� ���߿� �ʿ��ϸ�[5/12/2021 qwerwy]

	actor->rotateOnPlane(rotateAngle);
	return true;
}

bool StageManager::applyGravity(Actor* actor, const TickCount64& deltaTick) const noexcept
{

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
	if (findIt == _actionchartMap.end())
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
	check(sectorIndex < _actorsBySector.size(), "sector ������ ����ϴ�.");

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
	DirectX::XMFLOAT3 baseCoord = { static_cast<float>((_sectorUnitNumber.x / 2) * _sectorSize.x),
									static_cast<float>((_sectorUnitNumber.y / 2) * _sectorSize.y),
									static_cast<float>((_sectorUnitNumber.z / 2) * _sectorSize.z) };

	DirectX::XMINT3 rv = { static_cast<int>(position.x + baseCoord.x) % _sectorSize.x,
						   static_cast<int>(position.y + baseCoord.y) % _sectorSize.y,
						   static_cast<int>(position.z + baseCoord.z) % _sectorSize.z };
	return rv;
}

void StageManager::spawnActors()
{
	check(_stageInfo != nullptr);
	const auto& spawnInfos = _stageInfo->getSpawnInfos();
	for (const auto& spawnInfo : spawnInfos)
	{
		std::unique_ptr<Actor> actor(new Actor(spawnInfo));
		
		int sectorIndex = sectorCoordToIndex(getSectorCoord(spawnInfo._position));
		_actorsBySector[sectorIndex].emplace(actor.get());
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
