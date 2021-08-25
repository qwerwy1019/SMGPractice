#pragma once
#include "TypeD3d.h"
#include "TypeCommon.h"
#include "TypeGeometry.h"
#include "TypeStage.h"

class XMLReaderNode;
class CameraPoint;
class Path;

struct SpawnInfo
{
	SpawnInfo() noexcept;
	CharacterKey _key;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _upVector;
	float _size;
};

struct TerrainObjectInfo
{
	TerrainObjectInfo() noexcept;
	std::string _objectFileName;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _upVector;
	float _size;
	bool _isGround;
	bool _isWall;
};

struct GravityPoint
{
	GravityPoint() noexcept;
	int _key;
	GravityPointType _type;

	float _gravity;
	float _radius;

	DirectX::XMFLOAT3 _position;
};

class StageInfo
{
public:
	StageInfo(void) noexcept;
	~StageInfo();
	void loadXml(const XMLReaderNode& rootNode);
	void loadXmlSpawnInfo(const XMLReaderNode& node);
	void loadXmlTerrainObjectInfo(const XMLReaderNode& node);
	void loadXmlGravityPointInfo(const XMLReaderNode& node);
	void loadXmlLightInfo(const XMLReaderNode& node);
	void loadXmlEffectFiles(const XMLReaderNode& node);
	void loadXmlCameraInfo(const XMLReaderNode& node);
	void loadXmlTriggeredCameraInfo(const XMLReaderNode& node);
	void loadXmlAutoCameraInfo(const XMLReaderNode& node);
	void loadXmlPaths(const XMLReaderNode& node);

	std::vector<const CameraPoint*> getNearCameraPoints(const DirectX::XMFLOAT3& position) const noexcept;
	const CameraPoint* getTriggeredCameraPoint(int key) const noexcept;
	const std::vector<SpawnInfo>& getSpawnInfos(void) const noexcept;
	const std::vector<TerrainObjectInfo>& getTerrainObjectInfos(void) const noexcept;
	const DirectX::XMINT3& getSectorUnitNumber(void) const noexcept;
	const DirectX::XMINT3& getSectorSize(void) const noexcept;
	const GravityPoint* getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept;

	const std::vector<Light>& getLights(void) const noexcept;
	const DirectX::XMFLOAT4& getAmbientLight(void) const noexcept;

	const std::vector<std::string>& getEffectFileNames(void) const noexcept;
	const Path* getPath(int key) const noexcept;
private:
	LandscapeType _landscapeType;
	std::vector<std::unique_ptr<CameraPoint>> _autoCameraPoints;
	std::unordered_map<int, std::unique_ptr<CameraPoint>> _triggeredCmeraPoints;
	std::vector<SpawnInfo> _spawnInfo;
	std::vector<TerrainObjectInfo> _terrainObjectInfo;
	std::unordered_map<int, std::unique_ptr<GravityPoint>> _gravityPoints;
	std::vector<std::string> _effectFileNames;
	std::unordered_map<int, std::unique_ptr<Path>> _paths;

	DirectX::XMFLOAT4 _ambientLight;
	std::vector<Light> _lightInfo;

	DirectX::XMINT3 _sectorUnitNumber;
	DirectX::XMINT3 _sectorSize;

	std::string _name;
	
};