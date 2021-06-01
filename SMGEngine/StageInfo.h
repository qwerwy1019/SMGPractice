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
	bool _isTerrain;
};

struct CameraPoint
{
	int _index;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _upVector;
	DirectX::XMFLOAT3 _focusPosition;
	bool _hasFocusPosition;
	
	float _radius;
	bool _isAuto;
};

class StageInfo
{
public:
	void loadXml(const XMLReaderNode& rootNode);
	void loadXmlSpawnInfo(const XMLReaderNode& node);
	std::vector<CameraPoint*> getNearCameraPoints

	const std::vector<SpawnInfo>& getSpawnInfos(void) const noexcept;
private:
	LandscapeType _landscapeType;
	std::vector<std::unique_ptr<CameraPoint>> _cameraPoints;
	std::vector<SpawnInfo> _spawnInfo;

	std::string _name;
	
};