#include "stdafx.h"
#include "CharacterInfoManager.h"
#include "StageInfo.h"
#include "Actor.h"
#include "FileHelper.h"

CharacterInfoManager::CharacterInfoManager()
{
	loadXML();
}

const CharacterInfo* CharacterInfoManager::getInfo(const CharacterKey key) const
{
	auto it = _infos.find(key);
	if (it != _infos.end())
	{
		return &it->second;
	}
	else
	{
		return nullptr;
	}
}

void CharacterInfoManager::loadXML()
{
	const std::string characterInfoPath =
		"../Resources/XmlFiles/CharacterInfo.xml";
	XMLReader xmlStageInfo;

	xmlStageInfo.loadXMLFile(characterInfoPath);

	const auto& nodes = xmlStageInfo.getRootNode().getChildNodes();
	
	for (const auto& node : nodes)
	{
		CharacterKey key;
		node.loadAttribute("Key", key);
		_infos.emplace(key, node);
	}
}

CharacterInfo::CharacterInfo(const XMLReaderNode& node)
{
	node.loadAttribute("Name", _name);
	node.loadAttribute("ObjectFile", _objectFileName);
	node.loadAttribute("ActionChart", _actionChartFileName);

	std::string typeString;
	node.loadAttribute("CollisionType", typeString);
	if (typeString == "SolidObject")
	{
		_collisionType = CollisionType::SolidObject;
	}
	else if (typeString == "Character")
	{
		_collisionType = CollisionType::Character;
	}
	else if (typeString == "Item")
	{
		_collisionType = CollisionType::Item;
	}
	else
	{
		static_assert(static_cast<int>(CollisionType::Count) == 3, "Ÿ�� �߰��� Ȯ��");
		ThrowErrCode(ErrCode::UndefinedType, "collisionType Error : " + typeString);
	}

	std::string shapeString;
	node.loadAttribute("CollisionShape", _name);
	if (shapeString == "Sphere")
	{
		_collisionShape = CollisionShape::Sphere;
	}
	else if (shapeString == "Box")
	{
		_collisionShape = CollisionShape::Box;
	}
	else if (shapeString == "Polygon")
	{
		_collisionShape = CollisionShape::Polygon;
	}
	else
	{
		static_assert(static_cast<int>(CollisionShape::Count) == 3, "Ÿ�� �߰��� Ȯ��");
		ThrowErrCode(ErrCode::UndefinedType, "collisionShape Error : " + shapeString);
	}

	node.loadAttribute("Radius", _radius);
	if (_collisionShape == CollisionShape::Box)
	{
		node.loadAttribute("BoxSize", _boxSize);
	}
	else
	{
		_boxSize = { 0.f, 0.f, 0.f };
	}

	float boxRadiusSq = _boxSize.x * _boxSize.x + _boxSize.y * _boxSize.y + _boxSize.z * _boxSize.z;
	if (_radius * _radius < boxRadiusSq)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "radius�� boxSize���� Ŀ���մϴ�.");
	}
}