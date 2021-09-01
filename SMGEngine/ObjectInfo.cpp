#include "stdafx.h"
#include "ObjectInfo.h"
#include "FileHelper.h"

ObjectInfo::ObjectInfo(const XMLReaderNode& node)
{
	node.loadAttribute("Position", _position);
	node.loadAttribute("Direction", _direction);
	node.loadAttribute("UpVector", _upVector);
	node.loadAttribute("Size", _size);

	using namespace DirectX;
	XMStoreFloat3(&_upVector, DirectX::XMVector3Normalize(XMLoadFloat3(&_upVector)));
	XMStoreFloat3(&_direction, DirectX::XMVector3Normalize(XMLoadFloat3(&_direction)));
}

ObjectInfo::ObjectInfo(const DirectX::XMFLOAT3& position,
						const DirectX::XMFLOAT3& direction, 
						const DirectX::XMFLOAT3& upVector,
						float size) noexcept
	: _position(position)
	, _direction(direction)
	, _upVector(upVector)
	, _size(size)
{

}

BackgroundObjectInfo::BackgroundObjectInfo(const XMLReaderNode& node)
	: ObjectInfo(node)
{
	node.loadAttribute("ObjectFile", _objectFileName);
}

SpawnInfo::SpawnInfo(const XMLReaderNode& node)
	: ObjectInfo(node)
{
	node.loadAttribute("CharacterKey", _key);
	node.loadAttribute("ActionIndex", _actionIndex);
}

SpawnInfo::SpawnInfo(const DirectX::XMFLOAT3& position,
					const DirectX::XMFLOAT3& direction,
					const DirectX::XMFLOAT3& upVector,
					float size,
					CharacterKey key, 
					int actionIndex) noexcept
	: ObjectInfo(position, direction, upVector, size)
	, _key(key)
	, _actionIndex(actionIndex)
{

}

TerrainObjectInfo::TerrainObjectInfo(const XMLReaderNode& node)
	: ObjectInfo(node)
{
	node.loadAttribute("ObjectFile", _objectFileName);
	node.loadAttribute("IsGround", _isGround);
	node.loadAttribute("IsWall", _isWall);
}
