#include "stdafx.h"
#include "StageInfo.h"
#include "Exception.h"
#include "FileHelper.h"
#include "SMGFramework.h"
#include "CharacterInfoManager.h"
#include "MathHelper.h"

void StageInfo::loadXml(const XMLReaderNode& rootNode)
{
	rootNode.loadAttribute("Name", _name);
	rootNode.loadAttribute("SectorUnitNumber", _sectorUnitNumber);
	rootNode.loadAttribute("SectorSize", _sectorSize);

	const auto& childNodes = rootNode.getChildNodesWithName();
	auto childIter = childNodes.end();

	childIter = childNodes.find("Landscape");
	if (childIter == childNodes.end())
	{
		ThrowErrCode(ErrCode::NodeNotFound, "Landscape 노드가 없습니다.");
	}
	std::string typeString;
	childIter->second.loadAttribute("Type", typeString);
	if (typeString == "Basic")
	{
		_landscapeType = LandscapeType::Basic;
	}
	else if (typeString == "Galaxy")
	{
		_landscapeType = LandscapeType::Galaxy;
	}
	static_assert(static_cast<int>(LandscapeType::Count) == 2, "타입 추가시 확인");

	childIter = childNodes.find("SpawnInfo");
	loadXmlSpawnInfo(childIter->second);

	childIter = childNodes.find("TerrainObjects");
	loadXmlTerrainObjectInfo(childIter->second);

	childIter = childNodes.find("GravityPoints");
	loadXmlGravityPointInfo(childIter->second);
}

void StageInfo::loadXmlSpawnInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_spawnInfo.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("CharacterKey", _spawnInfo[i]._key);
		childNodes[i].loadAttribute("Position", _spawnInfo[i]._position);
		childNodes[i].loadAttribute("Direction", _spawnInfo[i]._direction);
		childNodes[i].loadAttribute("UpVector", _spawnInfo[i]._upVector);
		childNodes[i].loadAttribute("Size", _spawnInfo[i]._size);
	}
}

std::vector<CameraPoint*> StageInfo::getNearCameraPoints(const DirectX::XMFLOAT3& position) const noexcept
{
	std::vector<CameraPoint*> rv;
	
	for (const auto& camera : _cameraPoints)
	{
		float distanceSq = MathHelper::lengthSq(MathHelper::sub(position, camera->_position));
		if (distanceSq < camera->_radius * camera->_radius)
		{
			rv.push_back(camera.get());
		}
	}
	return rv;
}

const FixedCameraPoint& StageInfo::getFixedCameraPoint(const std::string& name) const noexcept
{
	auto it = _fixedCameraPoints.find(name);
	check(it != _fixedCameraPoints.end(), name);

	return *it->second.get();
}

const std::vector<SpawnInfo>& StageInfo::getSpawnInfos(void) const noexcept
{
	return _spawnInfo;
}

const std::vector<TerrainObjectInfo>& StageInfo::getTerrainObjectInfos(void) const noexcept
{
	return _terrainObjectInfo;
}

const DirectX::XMINT3& StageInfo::getSectorUnitNumber(void) const noexcept
{
	return _sectorUnitNumber;
}

const DirectX::XMINT3& StageInfo::getSectorSize(void) const noexcept
{
	return _sectorSize;
}

const GravityPoint* StageInfo::getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept
{
	using namespace MathHelper;
	for (const auto& gravityPoint : _gravityPoints)
	{
		if (lengthSq(sub(gravityPoint.second->_position, position)) <=
			gravityPoint.second->_radius * gravityPoint.second->_radius)
		{
			return gravityPoint.second.get();
		}
	}
	return nullptr;
}

void StageInfo::loadXmlTerrainObjectInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_terrainObjectInfo.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("ObjectFile", _terrainObjectInfo[i]._objectFileName);
		childNodes[i].loadAttribute("Position", _terrainObjectInfo[i]._position);
		childNodes[i].loadAttribute("Direction", _terrainObjectInfo[i]._direction);
		childNodes[i].loadAttribute("UpVector", _terrainObjectInfo[i]._upVector);
		childNodes[i].loadAttribute("Size", _terrainObjectInfo[i]._size);
		childNodes[i].loadAttribute("IsGround", _terrainObjectInfo[i]._isGround);
		childNodes[i].loadAttribute("IsWall", _terrainObjectInfo[i]._isWall);
	}
}

void StageInfo::loadXmlGravityPointInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_gravityPoints.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		std::unique_ptr<GravityPoint> gravityPoint = std::make_unique<GravityPoint>();
		int key;
		childNodes[i].loadAttribute("Key", key);
		gravityPoint->_key = key;
		std::string typeString;
		childNodes[i].loadAttribute("Type", typeString);
		if (typeString == "Fixed")
		{
			gravityPoint->_type = GravityPointType::Fixed;
		}
		else if (typeString == "Point")
		{
			gravityPoint->_type = GravityPointType::Point;
		}
		else if (typeString == "GroundNormal")
		{
			gravityPoint->_type = GravityPointType::GroundNormal;
		}
		else
		{
			ThrowErrCode(ErrCode::UndefinedType, typeString);
			static_assert(static_cast<int>(GravityPointType::Count) == 3, "타입 추가시 확인");
		}

		childNodes[i].loadAttribute("Gravity", gravityPoint->_gravity);
		childNodes[i].loadAttribute("Radius", gravityPoint->_radius);
		childNodes[i].loadAttribute("Position", gravityPoint->_position);

		_gravityPoints.emplace(key, std::move(gravityPoint));
	}
}
