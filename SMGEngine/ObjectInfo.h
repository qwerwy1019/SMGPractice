#pragma once
#include "TypeCommon.h"
class XMLReaderNode;

class ObjectInfo
{
public:
	ObjectInfo(const XMLReaderNode& node);
	ObjectInfo(const DirectX::XMFLOAT3& position,
				const DirectX::XMFLOAT3& direction,
				const DirectX::XMFLOAT3& upVector,
				float size) noexcept;
	const DirectX::XMFLOAT3& getPosition(void) const noexcept { return _position; }
	const DirectX::XMFLOAT3& getDirection(void) const noexcept { return _direction; }
	const DirectX::XMFLOAT3& getUpVector(void) const noexcept { return _upVector; }
	float getSize(void) const noexcept { return _size; }
private:
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _upVector;
	float _size;
};

class BackgroundObjectInfo : public ObjectInfo
{
public:
	BackgroundObjectInfo(const XMLReaderNode& node);
	const std::string& getObjectFileName(void) const noexcept { return _objectFileName; }
private:
	std::string _objectFileName;
};

class SpawnInfo : public ObjectInfo
{
public:
	SpawnInfo(const XMLReaderNode& node);
	SpawnInfo(const DirectX::XMFLOAT3& position,
				const DirectX::XMFLOAT3& direction,
				const DirectX::XMFLOAT3& upVector,
				float size,
				CharacterKey key,
				int actionIndex) noexcept;
	CharacterKey getCharacterKey(void) const noexcept { return _key; }
	int getActionIndex(void) const noexcept { return _actionIndex; }
private:
	CharacterKey _key;

	int _actionIndex;
};

class TerrainObjectInfo : public ObjectInfo
{
public:
	TerrainObjectInfo(const XMLReaderNode& node);
	const std::string getObjectFileName(void) const noexcept { return _objectFileName; }
	bool isGround(void) const noexcept { return _isGround; }
	bool isWall(void) const noexcept { return _isWall; }
private:
	std::string _objectFileName;

	bool _isGround;
	bool _isWall;
};
