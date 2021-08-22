#pragma once
#include "TypeStage.h"

struct CameraPointData
{
	CameraPointData(CameraPointType type, const XMLReaderNode& node);
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _upVector;
	DirectX::XMFLOAT3 _direction;
	float _distance;
};

class CameraPoint
{
public:
	CameraPoint(const XMLReaderNode& node);
	CameraPointType getType(void) const noexcept;
	bool checkInRange(const DirectX::XMFLOAT3& position) const noexcept;
	float getWeight(const DirectX::XMFLOAT3& position) const noexcept;
	const std::vector<CameraPointData>& getDatas(void) const noexcept;
private:
	std::vector<CameraPointData> _datas;
	DirectX::XMFLOAT3 _position;
	float _radius;
	CameraPointType _type;
};


// class FixedCameraPoint
// {
// 	FixedCameraPoint() noexcept;
// 	DirectX::XMFLOAT3 _position;
// 	DirectX::XMFLOAT3 _upVector;
// 	DirectX::XMFLOAT3 _focusPosition;
// 
// 	float _cameraSpeed;
// 	float _cameraFocusSpeed;
// };
