#pragma once
#include "TypeD3d.h"
#include "TypeCommon.h"

class XMLReaderNode;
class StageInfo
{
public:
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
	};
	void loadXml(const XMLReaderNode& rootNode);
	void loadXmlSpawnInfo(const XMLReaderNode& node);
private:
	LandscapeType _landscapeType;
	std::vector<SpawnInfo> _spawnInfo;

	std::string _name;
	
};