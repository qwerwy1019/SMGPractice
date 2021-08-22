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
	: _cameraInputPosition(100, 0, 0)
	, _cameraInputUpVector(0, 1, 0)
	, _cameraInputDirection(0, 0, -1)
	, _cameraDataIndexInput(0)
	, _cameraPositionSpeed(10.f)
	, _cameraAngleSpeed(0.01f)
	, _cameraPosition(0, 0, 3100)
	, _cameraUpVector(0, 1, 0)
	, _cameraDirection(0, 0, -1)
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
	XMVECTOR upVector = XMLoadFloat3(&_cameraUpVector);
	XMVECTOR direction = XMLoadFloat3(&_cameraDirection);

	XMVECTOR inputPosition = XMLoadFloat3(&_cameraInputPosition);
	XMVECTOR inputUpVector = XMLoadFloat3(&_cameraInputUpVector);
	XMVECTOR inputDirection = XMLoadFloat3(&_cameraInputDirection);

	float deltaMoveDistance = XMVectorGetX(XMVector3Length(position - inputPosition));
	float deltaAngle = std::max(XMVectorGetX(XMVector3AngleBetweenVectors(direction, inputDirection)),
						XMVectorGetX(XMVector3AngleBetweenVectors(upVector, inputUpVector)));

	if (deltaMoveDistance > _cameraPositionSpeed * deltaTickCount)
	{
		float t = _cameraPositionSpeed / static_cast<float>(deltaMoveDistance);
		position = XMVectorLerp(position, inputPosition, t);
	}
	else
	{
		position = inputPosition;
	}

	if (deltaAngle > _cameraAngleSpeed * deltaTickCount)
	{
		float t = _cameraAngleSpeed / static_cast<float>(deltaAngle);
		direction = XMVectorLerp(direction, inputDirection, t);
		upVector = XMVectorLerp(upVector, inputUpVector, t);
	}
	else
	{
		direction = inputDirection;
		upVector = inputUpVector;
	}

	XMVECTOR rightVector = XMVector3Cross(upVector, direction);
	upVector = XMVector3Cross(direction, rightVector);

	check(!isnan(XMVectorGetX(position)));
	check(!isnan(XMVectorGetX(upVector)));
	check(!isnan(XMVectorGetX(direction)));
	check(!isnan(XMVectorGetX(rightVector)));

	XMStoreFloat3(&_cameraPosition, position);
	XMStoreFloat3(&_cameraUpVector, upVector);
	XMStoreFloat3(&_cameraDirection, direction);
	XMStoreFloat3(&_cameraRightVector, rightVector);

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

	DirectX::XMFLOAT3 positionSum(0, 0, 0), upVectorSum(0, 0, 0), directionSum(0, 0, 0);
	float weightSum = 0.f;
	const auto& nearCameraList = stageInfo->getNearCameraPoints(playerPosition);
	int inputIndex = updateCameraDataIndexInput(nearCameraList);
	int newIndex = inputIndex;
	bool canSaveIndex = true;
	for (const auto& camera : nearCameraList)
	{
		const auto& cameraDatas = camera->getDatas();
		DirectX::XMFLOAT3 position, upVector, direction;
		if (cameraDatas.size() == 1)
		{
			getCameraData(cameraDatas[0], camera->getType(), position, upVector, direction);
		}
		else
		{
			if (inputIndex > 0)
			{
				getCameraData(cameraDatas[inputIndex], camera->getType(), position, upVector, direction);
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
				getCameraData(cameraDatas[nearestIndex], camera->getType(), position, upVector, direction);
			}
		}

		float cameraWeight = camera->getWeight(playerPosition);
		weightSum += cameraWeight;
		positionSum = MathHelper::add(positionSum, MathHelper::mul(position, cameraWeight));
		upVectorSum = MathHelper::add(upVectorSum, MathHelper::mul(upVector, cameraWeight));
		directionSum = MathHelper::add(directionSum, MathHelper::mul(direction, cameraWeight));
	}

	if (weightSum != 0.f)
	{
		_cameraInputPosition = MathHelper::div(positionSum, weightSum);
		_cameraInputUpVector = MathHelper::div(upVectorSum, weightSum);
		_cameraInputDirection = MathHelper::div(directionSum, weightSum);

		XMVECTOR direction = XMLoadFloat3(&_cameraInputDirection);
		XMVECTOR rightVector = XMVector3Cross(XMLoadFloat3(&_cameraInputUpVector), direction);
		XMVECTOR upVector = XMVector3Cross(direction, rightVector);
		XMStoreFloat3(&_cameraInputUpVector, upVector);
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

	int newIndex = std::clamp(_cameraDataIndex + _cameraDataIndexInput, 0, static_cast<int>(cameraDatas.size() - 1));
	_cameraDataIndexInput = 0;
	
	getCameraData(cameraDatas[newIndex], camera->getType(), _cameraInputPosition, _cameraInputUpVector, _cameraInputDirection);
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
	DirectX::XMFLOAT3& upVector,
	DirectX::XMFLOAT3& direction) const noexcept
{
	using namespace DirectX;
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);

	XMVECTOR playerPosition = XMLoadFloat3(&SMGFramework::getStageManager()->getPlayerActor()->getPosition());

	switch (type)
	{
		case CameraPointType::Fixed:
		case CameraPointType::PathCamera:
		{
			position = cameraData._position;
			upVector = cameraData._upVector;
			direction = cameraData._direction;
		}
		break;
		case CameraPointType::PlayerFocus:
		{
			upVector = cameraData._upVector;
			direction = cameraData._direction;
			
			XMVECTOR positionV = playerPosition - XMLoadFloat3(&direction) * cameraData._distance;
			XMStoreFloat3(&position, positionV);
		}
		break;
		case CameraPointType::PlayerFocusFixed:
		{
			position = cameraData._position;
			upVector = cameraData._upVector;

			XMVECTOR directionV = XMVector3Normalize(playerPosition - XMLoadFloat3(&position));
			XMStoreFloat3(&direction, directionV);
			
		}
		break;
		case CameraPointType::Count:
		default:
		{
			static_assert(static_cast<int>(CameraPointType::Count) == 4, "타입 추가시 확인");
			check(false, "타입 추가시 확인");
		}
	}
}

int Camera::getNearestCameraDataIndex(const CameraPoint* cameraPoint) const noexcept
{
	check(!cameraPoint->getDatas().empty());

	const auto& cameraDatas = cameraPoint->getDatas();
	float minDistance = FLT_MAX;
	int index = -1;
	for (int i=0;i<cameraDatas.size();++i)
	{
		DirectX::XMFLOAT3 position, upVector, direction;
		getCameraData(cameraDatas[i], cameraPoint->getType(), position, upVector, direction);

		float distance = MathHelper::length(MathHelper::sub(direction, _cameraInputDirection));
		//float distance = MathHelper::length(MathHelper::sub(position, _cameraInputPosition));
		if (distance < minDistance)
		{
			minDistance = distance;
			index = i;
		}
	}
	return index;
}
