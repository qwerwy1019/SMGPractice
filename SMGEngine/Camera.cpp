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
#include "D3DUtil.h"
#include "D3DApp.h"

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
#if defined DEBUG | defined _DEBUG
	, _camThetaDev(0)
	, _camPhiDev(0)
	, _camRadiusDev(200)
	, _useDevCam(false)
#endif
{

}

void Camera::releaseForStageLoad(void) noexcept
{
	_cameraPoint = nullptr;
	_blendTick = 0;
	_inputCameraPointKey = -1;
}

void Camera::update(void) noexcept
{
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getPlayerActor() != nullptr);

	TickCount64 deltaTickCount = SMGFramework::Get().getTimer().getDeltaTickCount();
	_currentTick += deltaTickCount;

	updateCameraPoint();
#if defined (DEBUG) | defined (_DEBUG)
	if (_useDevCam)
	{
		updateCameraPositionDev();
	}
	else
	{
		updateCameraPosition();
	}
#else
	updateCameraPosition();
#endif
	updatePassConstant();

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

DirectX::XMVECTOR Camera::getScreenPositionWorld(const DirectX::XMFLOAT2& screenPosition, float distance) const noexcept
{
	using namespace DirectX;

	float screenYSize = tanf(FOV_ANGLE * 0.5f) * 2.f * distance;
	float screenXSize = screenYSize * SMGFramework::getD3DApp()->getAspectRatio();

	float clientWidth = static_cast<float>(SMGFramework::Get().getClientWidth());
	float clientHeight = static_cast<float>(SMGFramework::Get().getClientHeight());

	float xOffset = (screenPosition.x - clientWidth / 2.f) / clientWidth; // -0.5 ~ 0.5
	float yOffset = (screenPosition.y - clientHeight / 2.f) / clientHeight; // -0.5 ~ 0.5
	
	XMVECTOR cameraUpVector = XMLoadFloat3(&_cameraUpVector);
	XMVECTOR cameraRightVector = XMLoadFloat3(&_cameraRightVector);

	XMVECTOR screenCenterWorld = XMLoadFloat3(&_cameraPosition) + XMLoadFloat3(&_cameraDirection) * NEAR_Z;

	XMVECTOR mousePositionWorld = screenCenterWorld 
								+ xOffset * XMLoadFloat3(&_cameraRightVector) *  screenXSize
								- yOffset * XMLoadFloat3(&_cameraUpVector) * screenYSize;
	XMFLOAT3 mousePositionWorldF;
	XMStoreFloat3(&mousePositionWorldF, mousePositionWorld);
	
	return mousePositionWorld;
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

void Camera::updatePassConstant() noexcept
{
	using namespace DirectX;

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

// 	std::string camInfo;
// 	camInfo += "pos : " + D3DUtil::toString(_cameraPosition, 0) + "\n";
// 	camInfo += "dir : " + D3DUtil::toString(_cameraDirection, 2) + "\n";
// 	camInfo += "up : " + D3DUtil::toString(_cameraUpVector, 2) + "\n\n";
// 	OutputDebugStringA(camInfo.c_str());

	XMMATRIX view = XMMatrixLookAtLH(positionV, positionV + direction, upVector);
	XMStoreFloat4x4(&_viewMatrix, view);
	XMVECTOR viewDet = XMMatrixDeterminant(view);
	XMStoreFloat4x4(&_invViewMatrix, XMMatrixInverse(&viewDet, view));
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

	bool success = _cameraPoint->getData(this, position, rotationQuat);
	if (!success)
	{
		return;
	}
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

#if defined DEBUG | _DEBUG
void Camera::updateCameraPositionDev(void) noexcept
{
	using namespace DirectX;
	XMVECTOR camPos = MathHelper::SphericalToCartesian(_camRadiusDev, _camPhiDev, _camThetaDev);
	XMVECTOR playerPos = XMLoadFloat3(&SMGFramework::getStageManager()->getPlayerActor()->getPosition());
	XMStoreFloat3(&_cameraPosition, camPos + playerPos);
	XMVECTOR upVector = XMVectorSet(0, 1, 0, 0);
	XMVECTOR direction = XMVector3Normalize(-camPos);
	XMVECTOR rotationQuat = MathHelper::getQuaternion(upVector, direction);
	XMStoreFloat4(&_cameraRotationQuat, rotationQuat);
}

void Camera::toggleDevCam(void) noexcept
{
	_useDevCam = !_useDevCam;
}

void Camera::addDevCamPhi(float delta) noexcept
{
	_camPhiDev += delta;
	_camPhiDev = std::clamp(_camPhiDev, 0.1f, MathHelper::Pi - 0.1f);
}

void Camera::addDevCamTheta(float delta) noexcept
{
	_camThetaDev += delta;
}

void Camera::addDevCamRadius(float delta) noexcept
{
	_camRadiusDev += delta;
	if (_camRadiusDev < 10)
	{
		_camRadiusDev = 10;
	}
}

#endif