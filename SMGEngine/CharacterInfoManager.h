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
	CollisionShape getCollisionShape(void) const noexcept { return _collisionShape; }
	CollisionType getCollisionType(void) const noexcept { return _collisionType; }
	float getRadius(void) const noexcept { return _radius; }
	float getSizeX(void) const noexcept { return _boxSize.x; }
	float getSizeY(void) const noexcept { return _boxSize.y; }
	float getSizeZ(void) const noexcept { return _boxSize.z; }
private:
	std::string _name;
	std::string _objectFileName;
	std::string _actionChartFileName;
	CollisionType _collisionType;
	CollisionShape _collisionShape;

	float _radius;

	// box type�϶��� ���Ǵ� �� [3/10/2021 qwerwy]
	DirectX::XMFLOAT3 _boxSize;

};
// info������ �������� template���� [5/20/2021 qwerwy]
class CharacterInfoManager
{
public:
	CharacterInfoManager();
	const CharacterInfo* getInfo(const CharacterKey key) const;
private:
	std::unordered_map<CharacterKey, CharacterInfo> _infos;
	void loadXML();
};