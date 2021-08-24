#pragma once
#include "TypeStage.h"

class CameraPoint;
struct CameraPointData;

class Camera
{
public:
	Camera();
	void update() noexcept;
	void updateByPlayerPosition() noexcept;
	void updateByCameraInput() noexcept;
	const DirectX::XMFLOAT4X4& getViewMatrix(void) const noexcept;
	const DirectX::XMFLOAT4X4& getInvViewMatrix(void) const noexcept;
	const DirectX::XMFLOAT3& getPosition(void) const noexcept;
	const DirectX::XMFLOAT3& getDirection(void) const noexcept;
	const DirectX::XMFLOAT3& getUpVector(void) const noexcept;
	const DirectX::XMFLOAT3& getRightVector(void) const noexcept;
	int updateCameraDataIndexInput(const std::vector<const CameraPoint*>& cameraList) noexcept;
	void getCameraData(const CameraPointData& cameraData, 
						CameraPointType type,
						DirectX::XMFLOAT3& position, 
						DirectX::XMFLOAT4& rotationQuat) const noexcept;
	int getNearestCameraDataIndex(const CameraPoint* cameraDatas) const noexcept;
private:
	// Ä«¸Þ¶ó
	DirectX::XMFLOAT3 _cameraInputPosition;
	//DirectX::XMFLOAT3 _cameraInputUpVector;
	//DirectX::XMFLOAT3 _cameraInputDirection;
	DirectX::XMFLOAT4 _cameraInputRotationQuat;
	int _cameraDataIndexInput;
	
	float _cameraPositionSpeed;
	float _cameraAngleSpeed;

	DirectX::XMFLOAT3 _cameraPosition;
	DirectX::XMFLOAT4 _cameraRotationQuat;
	DirectX::XMFLOAT3 _cameraUpVector;
	DirectX::XMFLOAT3 _cameraDirection;
	DirectX::XMFLOAT3 _cameraRightVector;
	int _cameraDataIndex;

	DirectX::XMFLOAT4X4 _viewMatrix;
	DirectX::XMFLOAT4X4 _invViewMatrix;

	int _inputCameraPointKey;
};