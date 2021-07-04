#pragma once
#include "TypeD3d.h"
#include "TypeCommon.h"

class XMLReaderNode;

enum class LandscapeType
{
	Basic,
	Galaxy,

	Count,
};

struct SpawnInfo
{
	CharacterKey _key;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _upVector;
	float _size;
};

struct TerrainObjectInfo
{
	std::string _objectFileName;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _upVector;
	float _size;
	bool _isGround;
	bool _isWall;
};

struct CameraPoint
{
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _upVector;
	float _radius;
};
struct FixedCameraPoint
{
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _upVector;
	DirectX::XMFLOAT3 _focusPosition;

	float _cameraSpeed;
	float _cameraFocusSpeed;
};

enum class GravityPointType
{
	Fixed,
	Point,
	GroundNormal,

	Count,
};

struct GravityPoint
{
	int _key;
	GravityPointType _type;

	float _gravity;
	float _radius;

	DirectX::XMFLOAT3 _position;
};

class StageInfo
{
public:
	void loadXml(const XMLReaderNode& rootNode);
	void loadXmlSpawnInfo(const XMLReaderNode& node);
	void loadXmlTerrainObjectInfo(const XMLReaderNode& node);
	void loadXmlGravityPointInfo(const XMLReaderNode& node);

	std::vector<CameraPoint*> getNearCameraPoints(const DirectX::XMFLOAT3& position) const noexcept;
	const FixedCameraPoint& getFixedCameraPoint(const std::string& name) const noexcept;
	const std::vector<SpawnInfo>& getSpawnInfos(void) const noexcept;
	const std::vector<TerrainObjectInfo>& getTerrainObjectInfos(void) const noexcept;
	const DirectX::XMINT3& getSectorUnitNumber(void) const noexcept;
	const DirectX::XMINT3& getSectorSize(void) const noexcept;
	const GravityPoint* getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept;
private:
	LandscapeType _landscapeType;
	std::vector<std::unique_ptr<CameraPoint>> _cameraPoints;
	std::unordered_map<std::string, std::unique_ptr<FixedCameraPoint>> _fixedCameraPoints;
	std::vector<SpawnInfo> _spawnInfo;
	std::vector<TerrainObjectInfo> _terrainObjectInfo;
	std::unordered_map<int, std::unique_ptr<GravityPoint>> _gravityPoints;

	DirectX::XMINT3 _sectorUnitNumber;
	DirectX::XMINT3 _sectorSize;

	std::string _name;
	
};