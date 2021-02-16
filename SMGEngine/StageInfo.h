#pragma once
#include "TypeD3d.h"
//#include <string>

enum class ErrCode : uint32_t;

using CharacterKey = uint16_t;
struct StageInfo
{
	enum class LandscapeType
	{
		Basic,
		Galaxy,

		Count,
	};
	enum class DirectionType
	{
		North,
		South,
		East,
		West,

		Count,
	};
	struct SpawnInfo
	{
		CharacterKey _key;
		DirectX::XMFLOAT3 _position;
		DirectionType _direction;
	};

	LandscapeType _landscapeType;
	std::vector<SpawnInfo> _spawnInfo;

	ErrCode loadXml(const std::string& fileName);
};