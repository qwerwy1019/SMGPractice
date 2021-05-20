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
		return it->second.get();
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
		std::unique_ptr<CharacterInfo> characterInfo(new CharacterInfo(node));
		_infos.emplace(key, std::move(characterInfo));
	}

}

CharacterInfo::CharacterInfo(const XMLReaderNode& node)
{
	node.loadAttribute("Name", _name);
	node.loadAttribute("ObjectFile", _objectFileName);
	
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
		static_assert(static_cast<int>(CollisionType::Count) == 3, "타입 추가시 확인");
		ThrowErrCode(ErrCode::UndefinedType, "collisionType Error : " + typeString);
	}
	
	std::string shapeString;
	node.loadAttribute("CollisionShape", _name);
	if (shapeString == "Sphere")
	{
		_collisionShape = CollisionShape::Sphere;
	}
	else if(shapeString == "Box")
	{
		_collisionShape = CollisionShape::Box;
	}
	else if (shapeString == "Polygon")
	{
		_collisionShape = CollisionShape::Polygon;
	}
	else
	{
		static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입 추가시 확인");
		ThrowErrCode(ErrCode::UndefinedType, "collisionShape Error : " + shapeString);
	}
}
