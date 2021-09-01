#include "stdafx.h"
#include "CameraPoint.h"
#include "FileHelper.h"
#include "MathHelper.h"
#include "SMGFramework.h"
#include "StageManager.h"
#include "Actor.h"
#include "Camera.h"
#include "GameObject.h"
#include "Path.h"

using namespace DirectX;

CameraPoint::CameraPoint(const XMLReaderNode& node, bool isAutoCamera)
	: _pointPosition(0, 0, 0)
	, _radius(0)
{
	if (isAutoCamera)
	{
		node.loadAttribute("PointPosition", _pointPosition);
		node.loadAttribute("Radius", _radius);
	}
	node.loadAttribute("BlendTick", _blendTick);
}

bool CameraPoint::checkInRange(const DirectX::XMFLOAT3& position) const noexcept
{
	float distanceSq = MathHelper::lengthSq(MathHelper::sub(position, _pointPosition));
	if (distanceSq < _radius * _radius)
	{
		return true;
	}
	return false;
}

float CameraPoint::getDistanceSq(const DirectX::XMFLOAT3& position) const noexcept
{
	float distance = MathHelper::lengthSq(MathHelper::sub(position, _pointPosition));
	return distance;
}

const TickCount64& CameraPoint::getBlendTick(void) const noexcept
{
	return _blendTick;
}

std::unique_ptr<CameraPoint> CameraPoint::loadXMLCameraPoint(const XMLReaderNode& node, bool isAutoCamera)
{
	std::string typeString;
	node.loadAttribute("Type", typeString);

	if (typeString == "Fixed")
	{
		return std::make_unique<CameraPoint_Fixed>(node, isAutoCamera);
	}
	else if (typeString == "PlayerFocus")
	{
		return std::make_unique<CameraPoint_PlayerFocus>(node, isAutoCamera);
	}
	else if (typeString == "PlayerFocusFixed")
	{
		return std::make_unique<CameraPoint_PlayerFocusFixed>(node, isAutoCamera);
	}
	else if (typeString == "Path")
	{
		return std::make_unique<CameraPoint_Path>(node, isAutoCamera);
	}
	else if (typeString == "CenterFocus")
	{
		return std::make_unique<CameraPoint_CenterFocus>(node, isAutoCamera);
	}
	else
	{
		ThrowErrCode(ErrCode::UndefinedType, typeString);
		static_assert(static_cast<int>(CameraPointType::Count) == 5, "타입 추가시 확인");
	}
}

CameraPoint_PlayerFocus::CameraPoint_PlayerFocus(const XMLReaderNode& node, bool isAutoCamera)
	: CameraPoint(node, isAutoCamera)
{
	node.loadAttribute("Offset", _offset);
	node.loadAttribute("IsCircular", _isCircular);

	const auto& childNodes = node.getChildNodes();
	_datas.reserve(childNodes.size());
	for (const auto& childNode : childNodes)
	{
		Data data;

		XMFLOAT3 upVector, direction;
		childNode.loadAttribute("UpVector", upVector);
		childNode.loadAttribute("Direction", direction);
		XMVECTOR rotationQuat = MathHelper::getQuaternion(XMLoadFloat3(&upVector), XMVector3Normalize(XMLoadFloat3(&direction)));
		XMStoreFloat4(&data._rotationQuat, rotationQuat);

		childNode.loadAttribute("Distance", data._distance);

		_datas.emplace_back(data);
	}
}

bool CameraPoint_PlayerFocus::getData(const Camera* camera, DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotationQuat) const noexcept
{
	check(camera != nullptr);
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor()->getGameObject() != nullptr);
	check(!_datas.empty());

	int index = camera->getCameraIndex();
	check(0 <= index && index < _datas.size());
	
	const auto& data = _datas[index];
	XMVECTOR rotationQuatV = XMLoadFloat4(&data._rotationQuat);
	XMVECTOR direction = XMVector3Rotate(XMVectorSet(0, 0, -1, 0), rotationQuatV);

	auto playerActor = SMGFramework::getStageManager()->getPlayerActor();

	XMVECTOR focusPosition = playerActor->getGameObject()->transformLocalToWorld(XMLoadFloat3(&_offset));
	XMStoreFloat3(&position, focusPosition - direction * data._distance);
	rotationQuat = data._rotationQuat;

	return true;
}

int CameraPoint_PlayerFocus::getNearestCameraDataIndex(const DirectX::XMFLOAT3& position,
													const DirectX::XMFLOAT4& rotationQuat) const noexcept
{
	check(!_datas.empty());
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor()->getGameObject() != nullptr);
	
	float minDistance = FLT_MAX;
	int index = -1;

	XMVECTOR positionV = XMLoadFloat3(&position);
	XMVECTOR rotationQuatV = XMLoadFloat4(&rotationQuat);
	XMVECTOR focusPosition = SMGFramework::getStageManager()->getPlayerActor()->getGameObject()->transformLocalToWorld(XMLoadFloat3(&_offset));
	
	for (int i = 0; i < _datas.size(); ++i)
	{
		XMVECTOR camDataRotationQuat = XMLoadFloat4(&_datas[i]._rotationQuat);
		XMVECTOR camDataDirection = XMVector3Rotate(XMVectorSet(0, 0, -1, 0), camDataRotationQuat);
		XMVECTOR camDataPosition = focusPosition - camDataDirection * _datas[i]._distance;
		float distance = XMVectorGetX(XMQuaternionLength(camDataRotationQuat - rotationQuatV));
		distance *= XMVectorGetX(XMVector3LengthEst(positionV - camDataPosition));
		if (distance < minDistance)
		{
			minDistance = distance;
			index = i;
		}
	}
	return index;
}

int CameraPoint_PlayerFocus::getNextIndex(int currentIndex, int direction) const noexcept
{
	check(0 <= currentIndex && currentIndex < _datas.size());
	check(direction == 1 || direction == -1);

	if (_isCircular)
	{
		return (currentIndex + direction + _datas.size()) % _datas.size();
	}
	else
	{
		if (currentIndex + direction < 0 || _datas.size() <= currentIndex + direction)
		{
			return -1;
		}
		return currentIndex + direction;
	}
}

CameraPoint_Fixed::CameraPoint_Fixed(const XMLReaderNode& node, bool isAutoCamera)
	: CameraPoint(node, isAutoCamera)
{
	node.loadAttribute("Position", _position);
	XMFLOAT3 upVector, direction;
	node.loadAttribute("UpVector", upVector);
	node.loadAttribute("Direction", direction);
	XMVECTOR rotationQuat = MathHelper::getQuaternion(XMLoadFloat3(&upVector), XMVector3Normalize(XMLoadFloat3(&direction)));
	XMStoreFloat4(&_rotationQuat, rotationQuat);
}

bool CameraPoint_Fixed::getData(const Camera* camera, DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotationQuat) const noexcept
{
	position = _position;
	rotationQuat = _rotationQuat;
	
	return true;
}

CameraPoint_PlayerFocusFixed::CameraPoint_PlayerFocusFixed(const XMLReaderNode& node, bool isAutoCamera)
	: CameraPoint(node, isAutoCamera)
{
	node.loadAttribute("Offset", _offset);
	node.loadAttribute("Position", _position);
	node.loadAttribute("UpVector", _upVector);

	XMStoreFloat3(&_upVector, DirectX::XMVector3Normalize(XMLoadFloat3(&_upVector)));
}

bool CameraPoint_PlayerFocusFixed::getData(const Camera* camera, DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotationQuat) const noexcept
{
	check(camera != nullptr);
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor()->getGameObject() != nullptr);

	XMVECTOR focusPosition = SMGFramework::getStageManager()->getPlayerActor()->getGameObject()->transformLocalToWorld(XMLoadFloat3(&_offset));
	XMVECTOR positionV = XMLoadFloat3(&_position);
	XMVECTOR direction = XMVector3Normalize(focusPosition - positionV);
	XMVECTOR upVector = XMLoadFloat3(&_upVector);

	XMVECTOR rotationQuatV = MathHelper::getQuaternion(upVector, direction);
	
	position = _position;
	XMStoreFloat4(&rotationQuat, rotationQuatV);
	
	return true;
}

CameraPoint_Path::CameraPoint_Path(const XMLReaderNode& node, bool isAutoCamera)
	: CameraPoint(node, isAutoCamera)
{
	_path = std::make_unique<Path>(node);
}

bool CameraPoint_Path::getData(const Camera* camera, DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotationQuat) const noexcept
{
	_path->getPathPositionAtTime(camera->getCurrentTick(), position);
	_path->getPathRotationAtTime(camera->getCurrentTick(), rotationQuat);

	return true;
}

CameraPoint_CenterFocus::CameraPoint_CenterFocus(const XMLReaderNode& node, bool isAutoCamera)
	: CameraPoint(node, isAutoCamera)
{
	node.loadAttribute("UpVector", _upVector);
	node.loadAttribute("Distance", _distance);
	node.loadAttribute("VerticalAngleRange", _verticalAngleRange);
	_verticalAngleRange *= MathHelper::Pi / 180.f;

	XMStoreFloat3(&_upVector, XMVector3Normalize(XMLoadFloat3(&_upVector)));
	if (!MathHelper::equal(MathHelper::length(_upVector), 1))
	{
		ThrowErrCode(ErrCode::InvalidXmlData);
	}
	if (_distance < 0)
	{
		ThrowErrCode(ErrCode::InvalidXmlData);
	}
	if (_verticalAngleRange < 0)
	{
		ThrowErrCode(ErrCode::InvalidXmlData);
	}
}

bool CameraPoint_CenterFocus::getData(const Camera* camera, DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotationQuat) const noexcept
{
	check(camera != nullptr);
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor()->getGameObject() != nullptr);

	XMVECTOR playerPosition = XMLoadFloat3(&SMGFramework::getStageManager()->getPlayerActor()->getPosition());
	XMVECTOR centerPosition = XMLoadFloat3(&_pointPosition);
	XMVECTOR upVector = XMLoadFloat3(&_upVector);
	XMVECTOR direction = XMVector3Normalize(centerPosition - playerPosition);

	float dot = std::clamp(XMVectorGetX(XMVector3Dot(upVector, direction)), -1.f, 1.f);
	float verticalAngle = MathHelper::Pi_DIV2 - std::acos(dot);
	if (dot == 0)
	{
		return false;
	}
	if (_verticalAngleRange < std::abs(verticalAngle))
	{
		float overAngle = (verticalAngle > 0) ? verticalAngle - _verticalAngleRange : verticalAngle + _verticalAngleRange;
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(upVector, direction));
		XMMATRIX rotationMatrix = XMMatrixRotationNormal(right, overAngle);
		direction = XMVector3Transform(direction, rotationMatrix);
	}
	
	XMStoreFloat3(&position, centerPosition - direction * _distance);
	XMStoreFloat4(&rotationQuat, MathHelper::getQuaternion(upVector, direction));
	return true;
}
