#pragma once
#include "TypeStage.h"

class CameraPoint;
struct CameraPointData;

class Camera
{
public:
	Camera();
	void releaseForStageLoad(void) noexcept;
	void update() noexcept;
	const DirectX::XMFLOAT4X4& getViewMatrix(void) const noexcept;
	const DirectX::XMFLOAT4X4& getInvViewMatrix(void) const noexcept;
	const DirectX::XMFLOAT3& getPosition(void) const noexcept;
	const DirectX::XMFLOAT3& getDirection(void) const noexcept;
	const DirectX::XMFLOAT3& getUpVector(void) const noexcept;
	const DirectX::XMFLOAT3& getRightVector(void) const noexcept;
	int getCameraIndex(void) const noexcept;
	const TickCount64& getCurrentTick(void) const noexcept;
	void setInputCameraPointKey(int key) noexcept;
	DirectX::XMVECTOR getScreenPositionWorld(const DirectX::XMFLOAT2& screenPosition, float distance) const noexcept;
private:
	void updateCameraPosition(void) noexcept;
	void updateCameraPoint(void) noexcept;
	void setCameraPoint(const CameraPoint* cameraPoint) noexcept;
	void setCameraIndex(int index) noexcept;
	int updateCameraInput(void) noexcept;
private:
	// Ä«¸Þ¶ó
	const CameraPoint* _cameraPoint;
	DirectX::XMFLOAT3 _cameraBlendPosition;
	DirectX::XMFLOAT4 _cameraBlendRotationQuat;
	TickCount64 _blendTick;
	TickCount64 _currentTick;
	
	float _cameraPositionSpeed;
	float _cameraAngleSpeed;

	DirectX::XMFLOAT3 _cameraPosition;
	DirectX::XMFLOAT4 _cameraRotationQuat;

	DirectX::XMFLOAT3 _cameraUpVector;
	DirectX::XMFLOAT3 _cameraDirection;
	DirectX::XMFLOAT3 _cameraRightVector;


	DirectX::XMFLOAT4X4 _viewMatrix;
	DirectX::XMFLOAT4X4 _invViewMatrix;

	int _inputCameraPointKey;
	// PlayerFocus
	int _cameraIndex;

	TickCount64 _keyInputTime;
	static constexpr TickCount64 KEY_INPUT_INTERVAL = 200;
	bool _cameraMoveFailed;
	void updatePassConstant() noexcept;
#if defined (DEBUG) | defined(_DEBUG)
public:
	void updateCameraPositionDev(void) noexcept;
	void toggleDevCam(void) noexcept;
	void addDevCamPhi(float delta) noexcept;
	void addDevCamTheta(float delta) noexcept;
	void addDevCamRadius(float delta) noexcept;
private:
	float _camThetaDev;
	float _camPhiDev;
	float _camRadiusDev;
	bool _useDevCam;
#endif
};