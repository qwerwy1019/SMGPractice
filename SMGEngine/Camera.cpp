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
	: _cameraPoint(nullptr)
	, _cameraBlendPosition(0, 0, 0)
	, _cameraBlendRotationQuat(0, 0, 0, 1)
	, _blendTick(0)
	, _currentTick(0)
	, _cameraPositionSpeed(0.2f)
	, _cameraAngleSpeed(0.003f)
	, _cameraPosition(0, 0, 0)
	, _cameraRotationQuat(0, 0, 0, 1)
	, _cameraUpVector(0, 1, 0)
	, _cameraDirection(0, 0, 1)
	, _cameraRightVector(1, 0, 0)
	, _invViewMatrix(MathHelper::Identity4x4)
	, _viewMatrix(MathHelper::Identity4x4)
	, _inputCameraPointKey(-1)
	, _cameraIndex(-1)
	, _keyInputTime(0)
	, _cameraMoveFailed(false)
{

}

void Camera::update(void) noexcept
{
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);

	TickCount64 deltaTickCount = SMGFramework::Get().getTimer().getDeltaTickCount();
	_currentTick += deltaTickCount;

	updateCameraPoint();
	updateCameraPosition();

	SMGFramework::getStageManager()->setCulled();
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

int Camera::getCameraIndex(void) const noexcept
{
	return _cameraIndex;
}

const TickCount64& Camera::getCurrentTick(void) const noexcept
{
	return _currentTick;
}

void Camera::setInputCameraPointKey(int key) noexcept
{
	_inputCameraPointKey = key;
}

void Camera::setCameraPoint(const CameraPoint* cameraPoint) noexcept
{
	check(cameraPoint != nullptr);
	if (cameraPoint == _cameraPoint)
	{
		return;
	}

	_blendTick = cameraPoint->getBlendTick();
	_cameraBlendPosition = _cameraPosition;
	_cameraBlendRotationQuat = _cameraRotationQuat;

	_currentTick = 0;

	if (cameraPoint->getType() == CameraPointType::PlayerFocus)
	{
		const CameraPoint_PlayerFocus* playerFocusCamera = static_cast<const CameraPoint_PlayerFocus*>(cameraPoint);
		_cameraIndex = playerFocusCamera->getNearestCameraDataIndex(_cameraPosition, _cameraRotationQuat);
	}

	_cameraPoint = cameraPoint;
}

void Camera::setCameraIndex(int index) noexcept
{
	check(_cameraPoint != nullptr);
	_blendTick = _cameraPoint->getBlendTick();
	_cameraBlendPosition = _cameraPosition;
	_cameraBlendRotationQuat = _cameraRotationQuat;

	_currentTick = 0;

	_cameraIndex = index;
}

int Camera::updateCameraInput(void) noexcept
{
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	if (currentTick < _keyInputTime + KEY_INPUT_INTERVAL)
	{
		return 0;
	}
	_cameraMoveFailed = false;
	const auto& stickInput = SMGFramework::Get().getStickInput(StickInputType::RStick);

	if (abs(stickInput.x) < 0.2f)
	{
		return 0;
	}


	if (stickInput.x < 0)
	{
		return -1; // left
	}
	else
	{
		return 1; //right
	}
	
	_keyInputTime = currentTick;
}

void Camera::updateCameraPosition(void) noexcept
{
	if (_cameraPoint == nullptr)
	{
		return;
	}

	using namespace DirectX;
	XMFLOAT3 position;
	XMFLOAT4 rotationQuat;
	_cameraPoint->getData(this, position, rotationQuat);

	if (_currentTick < _blendTick)
	{
		float t = static_cast<float>(_currentTick) / _blendTick;
		XMVECTOR positionV = XMVectorLerp(XMLoadFloat3(&_cameraBlendPosition), XMLoadFloat3(&position), t);
		XMVECTOR rotationQuatV = XMQuaternionSlerp(XMLoadFloat4(&_cameraBlendRotationQuat), XMLoadFloat4(&rotationQuat), t);

		XMStoreFloat3(&_cameraPosition, positionV);
		XMStoreFloat4(&_cameraRotationQuat, rotationQuatV);
	}
	else
	{
		_cameraPosition = position;
		_cameraRotationQuat = rotationQuat;
	}

	XMVECTOR positionV = XMLoadFloat3(&_cameraPosition);
	XMVECTOR rotationQuatV = XMLoadFloat4(&_cameraRotationQuat);
	XMVECTOR upVector, direction, rightVector;
	MathHelper::getRotatedAxis(rotationQuatV, upVector, direction, rightVector);

	check(!isnan(XMVectorGetX(upVector)));
	check(!isnan(XMVectorGetX(direction)));
	check(!isnan(XMVectorGetX(rightVector)));

	XMStoreFloat3(&_cameraUpVector, upVector);
	XMStoreFloat3(&_cameraDirection, direction);
	XMStoreFloat3(&_cameraRightVector, rightVector);

	XMMATRIX view = XMMatrixLookAtLH(positionV, positionV + direction, upVector);
	XMStoreFloat4x4(&_viewMatrix, view);
	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMStoreFloat4x4(&_invViewMatrix, XMMatrixInverse(&viewDet, view));
}

void Camera::updateCameraPoint(void) noexcept
{
	check(SMGFramework::getStageManager()->getStageInfo() != nullptr);
	using namespace DirectX;
	
	const CameraPoint* cameraPoint = nullptr;
	if (_inputCameraPointKey < 0)
	{
		XMFLOAT3 playerPosition = SMGFramework::getStageManager()->getPlayerActor()->getPosition();
		cameraPoint = SMGFramework::getStageManager()->getStageInfo()->getNearestCameraPoint(playerPosition);
	}
	else
	{
		cameraPoint = SMGFramework::getStageManager()->getStageInfo()->getTriggeredCameraPoint(_inputCameraPointKey);
	}

	if (cameraPoint == nullptr)
	{
		_inputCameraPointKey = -1;
		return;
	}
	setCameraPoint(cameraPoint);

	int input = updateCameraInput();
	if (input != 0 && _cameraPoint->getType() == CameraPointType::PlayerFocus)
	{
		const CameraPoint_PlayerFocus* playerFocusCamera = static_cast<const CameraPoint_PlayerFocus*>(_cameraPoint);
		int nextIndex = playerFocusCamera->getNextIndex(_cameraIndex, input);
		if (nextIndex < 0)
		{
			_cameraMoveFailed = true;
		}
		else
		{
			setCameraIndex(nextIndex);
		}
	}
}
