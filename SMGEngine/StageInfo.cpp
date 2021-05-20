#include "stdafx.h"
#include "StageInfo.h"
#include "Exception.h"
#include "FileHelper.h"

void StageInfo::loadXml(const XMLReaderNode& rootNode)
{
	rootNode.loadAttribute("Name", _name);

	const auto& childNodes = rootNode.getChildNodesWithName();
	auto childIter = childNodes.end();

	childIter = childNodes.find("Landscape");
	if (childIter == childNodes.end())
	{
		ThrowErrCode(ErrCode::NodeNotFound, "Landscape 노드가 없습니다.");
	}
	std::string typeString;
	childIter->second.loadAttribute("Type", typeString);
	if (typeString == "Basic")
	{
		_landscapeType = LandscapeType::Basic;
	}
	else if (typeString == "Galaxy")
	{
		_landscapeType = LandscapeType::Galaxy;
	}
	static_assert(static_cast<int>(LandscapeType::Count) == 2, "타입 추가시 확인");

	childIter = childNodes.find("SpawnInfo");
	loadXmlSpawnInfo(childIter->second);
}

void StageInfo::loadXmlSpawnInfo(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_spawnInfo.resize(childNodes.size());
	for (int i = 0; i < childNodes.size(); ++i)
	{
		childNodes[i].loadAttribute("CharacterKey", _spawnInfo[i]._key);
		childNodes[i].loadAttribute("Position", _spawnInfo[i]._position);
		childNodes[i].loadAttribute("Direction", _spawnInfo[i]._direction);
		childNodes[i].loadAttribute("UpVector", _spawnInfo[i]._upVector);
	}
}
