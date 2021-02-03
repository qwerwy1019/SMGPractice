#pragma once
#include "TypeData.h"

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

	HRESULT loadXml(const std::string& fileName);
};