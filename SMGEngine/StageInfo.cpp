#include "stdafx.h"
#include "StageInfo.h"
#include "Exception.h"
#include "FileHelper.h"
#include "MathHelper.h"
#include "CameraPoint.h"
#include "Path.h"
#include "ObjectInfo.h"

StageInfo::StageInfo(void) noexcept
	: _sectorUnitNumber(0, 0, 0)
	, _sectorSize(0, 0, 0)
	, _ambientLight(0, 0, 0, 0)
	, _backgroundColor(0, 0, 0)
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

	childIter = childNodes.find("StageScript");
	childIter->second.loadAttribute("Name", _stageScriptName);

	childIter = childNodes.find("SpawnInfo");
	loadXmlSpawnInfo(childIter->second);

	childIter = childNodes.find("SpawnInfoWithKey");
	loadXmlSpawnInfoWithKey(childIter->second);

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

	childIter = childNodes.find("Background");
	loadXmlBackgroundInfo(childIter->second);

	childIter = childNodes.find("UIInfo");
	loadXMLUIInfo(childIter->second);
}

void StageInfo::loadXmlBackgroundInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodesWithName();
	auto childIter = childNodes.end();

	childIter = childNodes.find("Color");
	childIter->second.loadAttribute("Value", _backgroundColor);

	childIter = childNodes.find("Objects");
	loadXmlBackgroundObjectInfo(childIter->second);
}

void StageInfo::loadXmlBackgroundObjectInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_backgroundObjectInfo.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		_backgroundObjectInfo.emplace_back(childNodes[i]);
	}
}

void StageInfo::loadXmlSpawnInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_spawnInfo.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		_spawnInfo.emplace_back(childNodes[i]);
	}
}

void StageInfo::loadXmlSpawnInfoWithKey(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_spawnInfoWithKey.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		int key;
		childNodes[i].loadAttribute("Key", key);
		_spawnInfoWithKey.emplace(key, childNodes[i]);
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

const CameraPoint* StageInfo::getNearestCameraPoint(const DirectX::XMFLOAT3& position) const noexcept
{
	float minDistanceSq = std::numeric_limits<float>::max();
	const CameraPoint* cameraPoint = nullptr;
	for (const auto& camera : _autoCameraPoints)
	{
		if (!camera->checkInRange(position))
		{
			continue;
		}
		float distanceSq = camera->getDistanceSq(position);
		if (distanceSq < minDistanceSq)
		{
			cameraPoint = camera.get();
			minDistanceSq = distanceSq;
		}
	}
	return cameraPoint;
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
	const GravityPoint* nearestPoint = nullptr;
	float nearestDistanceSq = std::numeric_limits<float>::max();
	for (const auto& gravityPoint : _gravityPoints)
	{
		float distanceSq = lengthSq(sub(gravityPoint.second->_position, position));
		if (distanceSq < nearestDistanceSq &&
			distanceSq <= gravityPoint.second->_radius * gravityPoint.second->_radius)
		{
			nearestDistanceSq = distanceSq;
			nearestPoint = gravityPoint.second.get();
		}
	}
	return nearestPoint;
}
const GravityPoint* StageInfo::getGravityPoint(int key) const noexcept
{
	auto it = _gravityPoints.find(key);
	if (it == _gravityPoints.end())
	{
		return nullptr;
	}
	return it->second.get();
}
const std::vector<Light>& StageInfo::getLights(void) const noexcept
{
	return _lightInfo;
}

const DirectX::XMFLOAT4& StageInfo::getAmbientLight(void) const noexcept
{
	return _ambientLight;
}

const std::vector<BackgroundObjectInfo>& StageInfo::getBackgroundObjectInfos(void) const noexcept
{
	return _backgroundObjectInfo;
}

const DirectX::XMFLOAT3& StageInfo::getBackgroundColor(void) const noexcept
{
	return _backgroundColor;
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

const SpawnInfo& StageInfo::getSpawnInfoWithKey(int key) const noexcept
{
	auto it = _spawnInfoWithKey.find(key);
	check(it != _spawnInfoWithKey.end());

	return it->second;
}

bool StageInfo::checkSpawnInfoWithKey(int key) const noexcept
{
	return _spawnInfoWithKey.find(key) != _spawnInfoWithKey.end();
}

const std::string& StageInfo::getStageScriptName(void) const noexcept
{
	return _stageScriptName;
}

const std::vector<std::string>& StageInfo::getUIFileNames(void) const noexcept
{
	return _uiFileNames;
}

void StageInfo::loadXmlTerrainObjectInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_terrainObjectInfo.reserve(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		_terrainObjectInfo.emplace_back(childNodes[i]);
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
		gravityPoint->_gravity *= ACCELERATION_UNIT;
		childNodes[i].loadAttribute("Radius", gravityPoint->_radius);
		childNodes[i].loadAttribute("MinRadius", gravityPoint->_minRadius);
		childNodes[i].loadAttribute("Position", gravityPoint->_position);
		if (gravityPoint->_type == GravityPointType::Fixed)
		{
			childNodes[i].loadAttribute("Direction", gravityPoint->_direction);
		}
		else
		{
			gravityPoint->_direction = { 0, 0, 0 };
		}

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
		_triggeredCmeraPoints.emplace(key, CameraPoint::loadXMLCameraPoint(childNode, false));
	}
}

void StageInfo::loadXmlAutoCameraInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		_autoCameraPoints.emplace_back(CameraPoint::loadXMLCameraPoint(childNode, true));
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

void StageInfo::loadXMLUIInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		std::string fileName;
		childNode.loadAttribute("FileName", fileName);

		_uiFileNames.emplace_back(fileName);
	}
}

GravityPoint::GravityPoint() noexcept
	: _key(-1)
	, _type(GravityPointType::Count)
	, _gravity(0.f)
	, _radius(0.f)
	, _position(0.f, 0.f, 0.f)
	, _minRadius(0.f)
{

}