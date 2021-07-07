#include "stdafx.h"
#include "CharacterInfoManager.h"
#include "StageInfo.h"
#include "Actor.h"
#include "FileHelper.h"
#include "MathHelper.h"

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

	std::string stringBuffer;

	node.loadAttribute("CharacterType", stringBuffer);
	if(stringBuffer == "Player")
	{
		_characterType = CharacterType::Player;
	}
	else if (stringBuffer == "Monster")
	{
		_characterType = CharacterType::Monster;
	}
	else if (stringBuffer == "Object")
	{
		_characterType = CharacterType::Object;
	}
	else
	{
		static_assert(static_cast<int>(CharacterType::Count) == 3, "타입 추가시 확인");
		ThrowErrCode(ErrCode::UndefinedType, "characterType Error : " + stringBuffer);
	}

	node.loadAttribute("CollisionType", stringBuffer);
	if (stringBuffer == "SolidObject")
	{
		_collisionType = CollisionType::SolidObject;
	}
	else if (stringBuffer == "Character")
	{
		_collisionType = CollisionType::Character;
	}
	else if (stringBuffer == "Item")
	{
		_collisionType = CollisionType::Item;
	}
	else
	{
		static_assert(static_cast<int>(CollisionType::Count) == 3, "타입 추가시 확인");
		ThrowErrCode(ErrCode::UndefinedType, "collisionType Error : " + stringBuffer);
	}

	node.loadAttribute("CollisionShape", stringBuffer);
	if (stringBuffer == "Sphere")
	{
		_collisionShape = CollisionShape::Sphere;
	}
	else if (stringBuffer == "Box")
	{
		_collisionShape = CollisionShape::Box;
	}
	else if (stringBuffer == "Polygon")
	{
		_collisionShape = CollisionShape::Polygon;
	}
	else
	{
		static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입 추가시 확인");
		ThrowErrCode(ErrCode::UndefinedType, "collisionShape Error : " + stringBuffer);
	}

	switch (_collisionShape)
	{
		case CollisionShape::Sphere:
		{
			_boxSize = { 0.f, 0.f, 0.f };
			node.loadAttribute("Radius", _radius);
		}
		break;
		case CollisionShape::Box:
		{
			node.loadAttribute("BoxSize", _boxSize);
			_radius = MathHelper::length(_boxSize);
		}
		break;
		case CollisionShape::Polygon:
		{
			check(false, "미구현");
			_boxSize = { 0.f, 0.f, 0.f };
			_radius = 0.f;
		}
		break;
		case CollisionShape::Count:
		default:
		{
			ThrowErrCode(ErrCode::UndefinedType, std::to_string(static_cast<int>(_collisionShape)));
			static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입 추가시 확인");
		}
		break;
	}
}
