#include "stdafx.h"
#include "Camera.h"
#include "MathHelper.h"
#include "TypeCommon.h"
#include "GameTimer.h"
#include "SMGFramework.h"
#include "StageManager.h"
#include "StageInfo.h"
#include "Actor.h"
#include "CameraPoint.h"

Camera::Camera()
	: _cameraInputPosition(0, 0, 0)
	, _cameraInputRotationQuat(0, 0, 0, 1)
	, _cameraDataIndexInput(0)
	, _cameraPositionSpeed(0.2f)
	, _cameraAngleSpeed(0.003f)
	, _cameraPosition(0, 0, 0)
	, _cameraRotationQuat(0, 0, 0, 1)
	, _cameraRightVector(1, 0, 0)
	, _cameraDataIndex(-1)
	, _invViewMatrix(MathHelper::Identity4x4)
	, _viewMatrix(MathHelper::Identity4x4)
	, _inputCameraPointKey(-1)
{

}

void Camera::update(void) noexcept
{
	if (_inputCameraPointKey < 0)
	{
		updateByPlayerPosition();
	}
	else
	{
		updateByCameraInput();
	}

	using namespace DirectX;

	TickCount64 deltaTickCount = SMGFramework::Get().getTimer().getDeltaTickCount();
	XMVECTOR position = XMLoadFloat3(&_cameraPosition);
	XMVECTOR rotationQuat = XMLoadFloat4(&_cameraRotationQuat);

	XMVECTOR inputPosition = XMLoadFloat3(&_cameraInputPosition);
	XMVECTOR inputRotationQuat = XMLoadFloat4(&_cameraInputRotationQuat);

	float deltaMoveDistance = XMVectorGetX(XMVector3Length(position - inputPosition));
	float deltaAngle = XMVectorGetX(XMQuaternionLength(rotationQuat - inputRotationQuat));
	
	float maxMoveDistance = _cameraPositionSpeed * deltaTickCount;
	float maxAngle = _cameraAngleSpeed * deltaTickCount;
	if (deltaMoveDistance > maxMoveDistance)
	{
		float t = maxMoveDistance / deltaMoveDistance;
		position = XMVectorLerp(position, inputPosition, t);
	}
	else
	{
		position = inputPosition;
	}

	if (deltaAngle > maxAngle)
	{
		float t = maxAngle / deltaAngle;
		rotationQuat = XMQuaternionSlerp(rotationQuat, inputRotationQuat, t);
	}
	else
	{
		rotationQuat = inputRotationQuat;
	}

	XMVECTOR upVector, direction, rightVector;
	MathHelper::getRotatedAxis(rotationQuat, upVector, direction, rightVector);
	
	check(!isnan(XMVectorGetX(position)));
	check(!isnan(XMVectorGetX(upVector)));
	check(!isnan(XMVectorGetX(direction)));
	check(!isnan(XMVectorGetX(rightVector)));

	XMStoreFloat3(&_cameraPosition, position);
	XMStoreFloat3(&_cameraUpVector, upVector);
	XMStoreFloat3(&_cameraDirection, direction);
	XMStoreFloat3(&_cameraRightVector, rightVector);
	XMStoreFloat4(&_cameraRotationQuat, rotationQuat);

	XMMATRIX view = XMMatrixLookAtLH(position, position + direction, upVector);
	XMStoreFloat4x4(&_viewMatrix, view);
	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMStoreFloat4x4(&_invViewMatrix, XMMatrixInverse(&viewDet, view));
}

void Camera::updateByPlayerPosition() noexcept
{
	using namespace DirectX;
	check(_inputCameraPointKey < 0);
	auto stageInfo = SMGFramework::getStageManager()->getStageInfo();
	check(stageInfo != nullptr);

	const auto& playerPosition = SMGFramework::getStageManager()->getPlayerActor()->getPosition();

	XMVECTOR positionSum = XMVectorZero();
	XMVECTOR rotationQuatAvg = XMVectorZero();
	float weightSum = 0.f;
	const auto& nearCameraList = stageInfo->getNearCameraPoints(playerPosition);
	int inputIndex = updateCameraDataIndexInput(nearCameraList);
	int newIndex = inputIndex;
	bool canSaveIndex = true;
	for (const auto& camera : nearCameraList)
	{
		const auto& cameraDatas = camera->getDatas();
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 rotationQuat;
		if (cameraDatas.size() == 1)
		{
			getCameraData(cameraDatas[0], camera->getType(), position, rotationQuat);
		}
		else
		{
			if (inputIndex > 0)
			{
				getCameraData(cameraDatas[inputIndex], camera->getType(), position, rotationQuat);
			}
			else
			{
				int nearestIndex = getNearestCameraDataIndex(camera);
				if (newIndex < 0)
				{
					newIndex = nearestIndex;
				}
				else
				{
					canSaveIndex = false;
					newIndex = -1;
				}
				getCameraData(cameraDatas[nearestIndex], camera->getType(), position, rotationQuat);
			}
		}

		float cameraWeight = camera->getWeight(playerPosition);
		weightSum += cameraWeight;

		check(cameraWeight > 0);
		positionSum += XMLoadFloat3(&position) * cameraWeight;
		rotationQuatAvg = XMQuaternionSlerp(rotationQuatAvg, XMLoadFloat4(&rotationQuat), cameraWeight / weightSum);
	}

	if (weightSum != 0.f)
	{
		XMStoreFloat3(&_cameraInputPosition, positionSum / weightSum);
		XMStoreFloat4(&_cameraInputRotationQuat, rotationQuatAvg);
	}
	if (canSaveIndex)
	{
		_cameraDataIndex = newIndex;
	}
}

void Camera::updateByCameraInput() noexcept
{
	check(_inputCameraPointKey >= 0);
	auto stageInfo = SMGFramework::getStageManager()->getStageInfo();
	check(stageInfo != nullptr);

	
	auto camera = stageInfo->getTriggeredCameraPoint(_inputCameraPointKey);
	const auto& cameraDatas = camera->getDatas();

	_cameraDataIndex = std::clamp(_cameraDataIndex + _cameraDataIndexInput, 0, static_cast<int>(cameraDatas.size() - 1));
	_cameraDataIndexInput = 0;
	
	getCameraData(cameraDatas[_cameraDataIndex], camera->getType(), _cameraInputPosition, _cameraInputRotationQuat);
}

const DirectX::XMFLOAT4X4& Camera::getViewMatrix(void) const noexcept
{
	return _viewMatrix;
}

const DirectX::XMFLOAT4X4& Camera::getInvViewMatrix(void) const noexcept
{
	return _invViewMatrix;
}

const DirectX::XMFLOAT3& Camera::getPosition(void) const noexcept
{
	return _cameraPosition;
}

const DirectX::XMFLOAT3& Camera::getDirection(void) const noexcept
{
	return _cameraDirection;
}

const DirectX::XMFLOAT3& Camera::getUpVector(void) const noexcept
{
	return _cameraUpVector;
}

const DirectX::XMFLOAT3& Camera::getRightVector(void) const noexcept
{
	return _cameraRightVector;
}

int Camera::updateCameraDataIndexInput(const std::vector<const CameraPoint*>& cameraList) noexcept
{
	int cameraDataCount = 1;
	int cameraDataInput = _cameraDataIndexInput;
	_cameraDataIndexInput = 0;

	if (_cameraDataIndexInput == 0)
	{
		return -1;
	}

	for (const auto& camera : cameraList)
	{
		int dataCount = camera->getDatas().size();
		if (dataCount != 1)
		{
			if (cameraDataCount != 1)
			{
				return -1;
			}
			cameraDataCount = dataCount;
		}
	}

	if (_cameraDataIndex < 0 || cameraDataCount <= _cameraDataIndex)
	{
		return -1;
	}
	int newIndex = _cameraDataIndex + cameraDataInput;
	if (newIndex < 0 || cameraDataCount <= newIndex)
	{
		return -1;
	}
	return newIndex;
}

void Camera::getCameraData(const CameraPointData& cameraData,
	CameraPointType type,
	DirectX::XMFLOAT3& position,
	DirectX::XMFLOAT4& rotationQuat) const noexcept
{
	using namespace DirectX;
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);

	XMVECTOR playerPosition = XMLoadFloat3(&SMGFramework::getStageManager()->getPlayerActor()->getPosition());
	XMVECTOR upVector, direction, rightVector;
	switch (type)
	{
		case CameraPointType::Fixed:
		case CameraPointType::PathCamera:
		{
			position = cameraData._position;
			upVector = XMLoadFloat3(&cameraData._upVector);
			direction = XMLoadFloat3(&cameraData._direction);
		}
		break;
		case CameraPointType::PlayerFocus:
		{
			upVector = XMLoadFloat3(&cameraData._upVector);
			direction = XMLoadFloat3(&cameraData._direction);
			
			XMVECTOR positionV = playerPosition - direction * cameraData._distance;
			XMStoreFloat3(&position, positionV);
		}
		break;
		case CameraPointType::PlayerFocusFixed:
		{
			position = cameraData._position;
			upVector = XMLoadFloat3(&cameraData._upVector);

			direction = XMVector3Normalize(playerPosition - XMLoadFloat3(&position));
		}
		break;
		case CameraPointType::Count:
		default:
		{
			static_assert(static_cast<int>(CameraPointType::Count) == 4, "타입 추가시 확인");
			check(false, "타입 추가시 확인");
		}
	}

	XMVECTOR rotationQuatV = MathHelper::getQuaternion(upVector, direction);
	
	XMStoreFloat4(&rotationQuat, rotationQuatV);
}

int Camera::getNearestCameraDataIndex(const CameraPoint* cameraPoint) const noexcept
{
	check(!cameraPoint->getDatas().empty());

	const auto& cameraDatas = cameraPoint->getDatas();
	float minDistance = FLT_MAX;
	int index = -1;
	for (int i=0;i<cameraDatas.size();++i)
	{
		using namespace DirectX;
		XMFLOAT3 position;
		XMFLOAT4 rotationQuat;
		getCameraData(cameraDatas[i], cameraPoint->getType(), position, rotationQuat);

		float distance = XMVectorGetX(XMQuaternionLength(XMLoadFloat4(&rotationQuat) - XMLoadFloat4(&_cameraInputRotationQuat)));
		distance *= MathHelper::length(MathHelper::sub(position, _cameraInputPosition));
		if (distance < minDistance)
		{
			minDistance = distance;
			index = i;
		}
	}
	return index;
}
