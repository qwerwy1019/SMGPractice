#include "stdafx.h"
#include "CameraPoint.h"
#include "FileHelper.h"
#include "MathHelper.h"

CameraPoint::CameraPoint(const XMLReaderNode& node)
{
	std::string typeString;
	node.loadAttribute("Type", typeString);
	if (typeString == "Fixed")
	{
		_type = CameraPointType::Fixed;
	}
	else if (typeString == "PlayerFocus")
	{
		_type = CameraPointType::PlayerFocus;
	}
	else if (typeString == "PlayerFocusFixed")
	{
		_type = CameraPointType::PlayerFocusFixed;
	}
	else if (typeString == "PathCamera")
	{
		_type = CameraPointType::PathCamera;
	}
	else
	{
		ThrowErrCode(ErrCode::UndefinedType, typeString);
		static_assert(static_cast<int>(CameraPointType::Count) == 4, "타입 추가시 확인");
	}

	node.loadAttribute("Position", _position);
	node.loadAttribute("Radius", _radius);
	const auto& childNodes = node.getChildNodes();
	if (childNodes.empty())
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "camera data가 없음");
	}
	for (const auto& childNode : childNodes)
	{
		_datas.emplace_back(_type, childNode);
	}
}

CameraPointType CameraPoint::getType(void) const noexcept
{
	return _type;
}

bool CameraPoint::checkInRange(const DirectX::XMFLOAT3& position) const noexcept
{
	float distanceSq = MathHelper::lengthSq(MathHelper::sub(position, _position));
	if (distanceSq < _radius * _radius)
	{
		return true;
	}
	return false;
}

float CameraPoint::getWeight(const DirectX::XMFLOAT3& position) const noexcept
{
	float distance = MathHelper::length(MathHelper::sub(position, _position));
	return (_radius - distance) * (_radius - distance) / (_radius * _radius);
}

const std::vector<CameraPointData>& CameraPoint::getDatas(void) const noexcept
{
	return _datas;
}

CameraPointData::CameraPointData(CameraPointType type, const XMLReaderNode& node)
	: _position(0, 0, 0)
	, _upVector(0, 1, 0)
	, _direction(0, 0, -1)
	, _distance(0)
{
	using namespace DirectX;
	switch (type)
	{
		case CameraPointType::Fixed:
		case CameraPointType::PathCamera:
		{
			node.loadAttribute("Position", _position);
			node.loadAttribute("UpVector", _upVector);
			node.loadAttribute("Direction", _direction);
			XMVECTOR direction = XMVector3Normalize(XMLoadFloat3(&_direction));
			XMVECTOR rightVector = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&_upVector), direction));
			XMVECTOR upVector = XMVector3Cross(direction, rightVector);
			XMStoreFloat3(&_upVector, upVector);
			XMStoreFloat3(&_direction, direction);
		}
		break;
		case CameraPointType::PlayerFocus:
		{
			node.loadAttribute("UpVector", _upVector);
			node.loadAttribute("Direction", _direction);
			node.loadAttribute("Distance", _distance);

			XMVECTOR direction = XMVector3Normalize(XMLoadFloat3(&_direction));
			XMVECTOR rightVector = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&_upVector), direction));
			XMVECTOR upVector = XMVector3Cross(direction, rightVector);
			XMStoreFloat3(&_upVector, upVector);
			XMStoreFloat3(&_direction, direction);
		}
		break;
		case CameraPointType::PlayerFocusFixed:
		{
			node.loadAttribute("Position", _position);
			node.loadAttribute("UpVector", _upVector);
		}
		break;
		case CameraPointType::Count:
		default:
		{
			ThrowErrCode(ErrCode::UndefinedType, std::to_string(static_cast<int>(type)));
			static_assert(static_cast<int>(CameraPointType::Count) == 4, "타입 추가시 확인");
		}
		break;
	}
}
