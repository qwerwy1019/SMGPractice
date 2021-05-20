#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"

class XMLReaderNode;

class CharacterInfo
{
public:
	CharacterInfo(const XMLReaderNode& node);

	CollisionShape getCollisionShape(void) const noexcept { return _collisionShape; }
	CollisionType getCollisionType(void) const noexcept { return _collisionType; }
	float getRadius(void) const noexcept { return _radius; }
	float getSizeX(void) const noexcept { return _radius; }
	float getSizeY(void) const noexcept { return _radius; }
	float getSizeZ(void) const noexcept { return _radius; }
private:
	std::string _name;
	std::string _objectFileName;
	CollisionType _collisionType;
	CollisionShape _collisionShape;

	float _radius;

	// box type일때만 사용되는 값 [3/10/2021 qwerwy]
	float _sizeX;
	float _sizeY;
	float _sizeZ;

};
// info종류가 많아지면 template으로 [5/20/2021 qwerwy]
class CharacterInfoManager
{
public:
	CharacterInfoManager();
	const CharacterInfo* getInfo(const CharacterKey key) const;
private:
	std::unordered_map<CharacterKey, std::unique_ptr<CharacterInfo>> _infos;
	void loadXML();
};