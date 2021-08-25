#include "stdafx.h"
#include "StageInfo.h"
#include "Exception.h"
#include "FileHelper.h"
#include "MathHelper.h"
#include "CameraPoint.h"
#include "Path.h"

StageInfo::StageInfo(void) noexcept
	: _landscapeType(LandscapeType::Basic)
	, _sectorUnitNumber(0, 0, 0)
	, _sectorSize(0, 0, 0)
	, _ambientLight(0, 0, 0, 0)
{

}

StageInfo::~StageInfo()
{

}

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

	childIter = childNodes.find("Lights");
	loadXmlLightInfo(childIter->second);

	childIter = childNodes.find("EffectFiles");
	loadXmlEffectFiles(childIter->second);

	childIter = childNodes.find("CameraPoints");
	loadXmlCameraInfo(childIter->second);

	childIter = childNodes.find("Paths");
	loadXmlPaths(childIter->second);
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

const std::vector<SpawnInfo>& StageInfo::getSpawnInfos(void) const noexcept
{
	return _spawnInfo;
}

std::vector<const CameraPoint*> StageInfo::getNearCameraPoints(const DirectX::XMFLOAT3& position) const noexcept
{
	std::vector<const CameraPoint*> rv;
	for (const auto& camera : _autoCameraPoints)
	{
		if (camera->checkInRange(position))
		{
			rv.push_back(camera.get());
		}
	}
	return rv;
}

const CameraPoint* StageInfo::getTriggeredCameraPoint(int key) const noexcept
{
	auto it = _triggeredCmeraPoints.find(key);
	if (it == _triggeredCmeraPoints.end())
	{
		return nullptr;
	}
	return it->second.get();
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

const std::vector<Light>& StageInfo::getLights(void) const noexcept
{
	return _lightInfo;
}

const DirectX::XMFLOAT4& StageInfo::getAmbientLight(void) const noexcept
{
	return _ambientLight;
}

const std::vector<std::string>& StageInfo::getEffectFileNames(void) const noexcept
{
	return _effectFileNames;
}

const Path* StageInfo::getPath(int key) const noexcept
{
	auto it = _paths.find(key);
	if (it == _paths.end())
	{
		return nullptr;
	}
	return it->second.get();
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
		gravityPoint->_gravity *= 0.001f;//ACCELERATION_UNIT;
		childNodes[i].loadAttribute("Radius", gravityPoint->_radius);
		childNodes[i].loadAttribute("Position", gravityPoint->_position);

		_gravityPoints.emplace(key, std::move(gravityPoint));
	}
}

void StageInfo::loadXmlLightInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_lightInfo.reserve(childNodes.size());
	if (childNodes.size() > MAX_LIGHT_COUNT || childNodes.size() == 0)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, std::to_string(MAX_LIGHT_COUNT) + " : " + std::to_string(childNodes.size()));
	}
	for (int i = 0; i < childNodes.size(); ++i)
	{
		Light light;
		std::string typeString;
		childNodes[i].loadAttribute("Type", typeString);
		if (typeString == "Ambient")
		{
			childNodes[i].loadAttribute("Strength", _ambientLight);
			continue;
		}
		else if (typeString == "Directional")
		{
			childNodes[i].loadAttribute("Strength", light._strength);
			childNodes[i].loadAttribute("Direction", light._direction);
		}
		else if (typeString == "Point")
		{
			childNodes[i].loadAttribute("Strength", light._strength);
			childNodes[i].loadAttribute("Position", light._position);
			childNodes[i].loadAttribute("FallOffStart", light._falloffStart);
			childNodes[i].loadAttribute("FallOffEnd", light._falloffEnd);
		}
		else if (typeString == "Spot")
		{
			childNodes[i].loadAttribute("Strength", light._strength);
			childNodes[i].loadAttribute("Direction", light._direction);
			childNodes[i].loadAttribute("Position", light._position);
			childNodes[i].loadAttribute("FallOffStart", light._falloffStart);
			childNodes[i].loadAttribute("FallOffEnd", light._falloffEnd);
			childNodes[i].loadAttribute("SpotPower", light._spotPower);
		}
		else
		{
			ThrowErrCode(ErrCode::UndefinedType, typeString);
		}
		
		
		_lightInfo.push_back(std::move(light));
	}
}

void StageInfo::loadXmlEffectFiles(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_effectFileNames.reserve(childNodes.size());
	for (const auto& childNode : childNodes)
	{
		std::string fileName;
		childNode.loadAttribute("FileName", fileName);
		_effectFileNames.push_back(fileName);
	}
}

void StageInfo::loadXmlCameraInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodesWithName();

	auto childIter = childNodes.find("TriggeredCamera");
	loadXmlTriggeredCameraInfo(childIter->second);

	childIter = childNodes.find("AutoCamera");
	loadXmlAutoCameraInfo(childIter->second);
}

void StageInfo::loadXmlTriggeredCameraInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		int key;
		childNode.loadAttribute("Key", key);
		if (key < 0)
		{
			ThrowErrCode(ErrCode::InvalidXmlData, std::to_string(key));
		}
		_triggeredCmeraPoints.emplace(key, std::make_unique<CameraPoint>(childNode));
	}
}

void StageInfo::loadXmlAutoCameraInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		_autoCameraPoints.emplace_back(std::make_unique<CameraPoint>(childNode));
	}
}

void StageInfo::loadXmlPaths(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		int key;
		childNode.loadAttribute("Key", key);
		_paths.emplace(key, std::make_unique<Path>(childNode));
	}
}

SpawnInfo::SpawnInfo() noexcept
	: _key(std::numeric_limits<CharacterKey>::max())
	, _position(0.f, 0.f, 0.f)
	, _direction(0.f, 0.f, 0.f)
	, _upVector(0.f, 0.f, 0.f)
	, _size(0.f)
{

}

TerrainObjectInfo::TerrainObjectInfo() noexcept
	: _position(0.f, 0.f, 0.f)
	, _direction(0.f, 0.f, 0.f)
	, _upVector(0.f, 0.f, 0.f)
	, _size(0.f)
	, _isGround(false)
	, _isWall(false)
{

}

GravityPoint::GravityPoint() noexcept
	: _key(-1)
	, _type(GravityPointType::Count)
	, _gravity(0.f)
	, _radius(0.f)
	, _position(0.f, 0.f, 0.f)
{

}
