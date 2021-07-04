#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"

class XMLReaderNode;

class CharacterInfo
{
public:
	CharacterInfo(const XMLReaderNode& node);

	const std::string& getName() const noexcept { return _name; }
	const std::string& getObjectFileName() const noexcept { return _objectFileName; }
	const std::string& getActionChartFileName() const noexcept { return _actionChartFileName; }
	CharacterType getCharacterType(void) const noexcept { return _characterType; }
	CollisionShape getCollisionShape(void) const noexcept { return _collisionShape; }
	CollisionType getCollisionType(void) const noexcept { return _collisionType; }
	float getRadiusXXX(void) const noexcept { return _radius; }
	float getSizeXXXX(void) const noexcept { return _boxSize.x; }
	float getSizeYXXX(void) const noexcept { return _boxSize.y; }
	float getSizeZXXX(void) const noexcept { return _boxSize.z; }
private:
	std::string _name;
	std::string _objectFileName;
	std::string _actionChartFileName;
	CharacterType _characterType;

	CollisionType _collisionType;
	CollisionShape _collisionShape;

	float _radius;

	// box type일때만 사용되는 값 [3/10/2021 qwerwy]
	DirectX::XMFLOAT3 _boxSize;

};
// info종류가 많아지면 template으로 [5/20/2021 qwerwy]
class CharacterInfoManager
{
public:
	CharacterInfoManager();
	const CharacterInfo* getInfo(const CharacterKey key) const;
private:
	std::unordered_map<CharacterKey, CharacterInfo> _infos;
	void loadXML();
};