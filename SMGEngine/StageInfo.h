#pragma once
#include "TypeD3d.h"
#include "TypeCommon.h"
#include "TypeGeometry.h"
#include "TypeStage.h"

class XMLReaderNode;
class CameraPoint;
class Path;

class ObjectInfo;
class TerrainObjectInfo;
class SpawnInfo;
class BackgroundObjectInfo;

struct GravityPoint
{
	GravityPoint() noexcept;
	int _key;
	GravityPointType _type;

	float _gravity;
	float _radius;
	float _minRadius;

	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _direction;
};

class StageInfo
{
public:
	StageInfo(void) noexcept;
	~StageInfo();
	void loadXml(const XMLReaderNode& rootNode);
	void loadXmlBackgroundInfo(const XMLReaderNode& node);
	void loadXmlBackgroundObjectInfo(const XMLReaderNode& node);
	void loadXmlSpawnInfo(const XMLReaderNode& node);
	void loadXmlSpawnInfoWithKey(const XMLReaderNode& node);
	void loadXmlTerrainObjectInfo(const XMLReaderNode& node);
	void loadXmlGravityPointInfo(const XMLReaderNode& node);
	void loadXmlLightInfo(const XMLReaderNode& node);
	void loadXmlEffectFiles(const XMLReaderNode& node);
	void loadXmlCameraInfo(const XMLReaderNode& node);
	void loadXmlTriggeredCameraInfo(const XMLReaderNode& node);
	void loadXmlAutoCameraInfo(const XMLReaderNode& node);
	void loadXmlPaths(const XMLReaderNode& node);
	void loadXMLUIInfo(const XMLReaderNode& node);

	std::vector<const CameraPoint*> getNearCameraPoints(const DirectX::XMFLOAT3& position) const noexcept;
	const CameraPoint* getNearestCameraPoint(const DirectX::XMFLOAT3& position) const noexcept;
	const CameraPoint* getTriggeredCameraPoint(int key) const noexcept;
	const std::vector<SpawnInfo>& getSpawnInfos(void) const noexcept;
	const std::vector<TerrainObjectInfo>& getTerrainObjectInfos(void) const noexcept;
	const DirectX::XMINT3& getSectorUnitNumber(void) const noexcept;
	const DirectX::XMINT3& getSectorSize(void) const noexcept;
	const GravityPoint* getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept;
	const GravityPoint* getGravityPoint(int key) const noexcept;
	const std::vector<Light>& getLights(void) const noexcept;
	const DirectX::XMFLOAT4& getAmbientLight(void) const noexcept;
	const std::vector<BackgroundObjectInfo>& getBackgroundObjectInfos(void) const noexcept;
	const DirectX::XMFLOAT3& getBackgroundColor(void) const noexcept;

	const std::vector<std::string>& getEffectFileNames(void) const noexcept;
	const Path* getPath(int key) const noexcept;
	const SpawnInfo& getSpawnInfoWithKey(int key) const noexcept;
	bool checkSpawnInfoWithKey(int key) const noexcept;
	const std::string& getStageScriptName(void) const noexcept;
	const std::vector<std::string>& getUIFileNames(void) const noexcept;
private:
	std::vector<std::unique_ptr<CameraPoint>> _autoCameraPoints;
	std::unordered_map<int, std::unique_ptr<CameraPoint>> _triggeredCmeraPoints;
	std::vector<SpawnInfo> _spawnInfo;
	std::unordered_map<int, SpawnInfo> _spawnInfoWithKey;
	std::vector<TerrainObjectInfo> _terrainObjectInfo;
	std::vector<BackgroundObjectInfo> _backgroundObjectInfo;
	std::unordered_map<int, std::unique_ptr<GravityPoint>> _gravityPoints;
	std::vector<std::string> _effectFileNames;
	std::unordered_map<int, std::unique_ptr<Path>> _paths;
	std::string _stageScriptName;
	std::vector<std::string> _uiFileNames;

	DirectX::XMFLOAT4 _ambientLight;
	DirectX::XMFLOAT3 _backgroundColor;
	std::vector<Light> _lightInfo;

	DirectX::XMINT3 _sectorUnitNumber;
	DirectX::XMINT3 _sectorSize;

	std::string _name;
	
};