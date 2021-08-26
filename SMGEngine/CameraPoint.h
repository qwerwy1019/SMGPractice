#pragma once
#include "TypeStage.h"

class Camera;
class Path;

class CameraPoint
{
public:
	CameraPoint(const XMLReaderNode& node, bool isAutoCamera);
	virtual ~CameraPoint() = default;
	virtual CameraPointType getType(void) const noexcept = 0;
	virtual void getData(const Camera* camera,
					DirectX::XMFLOAT3& position,
					DirectX::XMFLOAT4& rotationQuat) const noexcept = 0;
	bool checkInRange(const DirectX::XMFLOAT3& position) const noexcept;
	float getDistanceSq(const DirectX::XMFLOAT3& position) const noexcept;
	const TickCount64& getBlendTick(void) const noexcept;
	static std::unique_ptr<CameraPoint> loadXMLCameraPoint(const XMLReaderNode& node, bool isAutoCamera);
private:
	DirectX::XMFLOAT3 _pointPosition;
	float _radius;
	TickCount64 _blendTick;
};

class CameraPoint_Fixed : public CameraPoint
{
public:
	CameraPoint_Fixed(const XMLReaderNode& node, bool isAutoCamera);
	virtual ~CameraPoint_Fixed() = default;
	virtual CameraPointType getType(void) const noexcept override { return CameraPointType::Fixed; }
	virtual void getData(const Camera* camera,
					DirectX::XMFLOAT3& position,
					DirectX::XMFLOAT4& rotationQuat) const noexcept override;
private:
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT4 _rotationQuat;
};

class CameraPoint_PlayerFocus : public CameraPoint
{
public:
	CameraPoint_PlayerFocus(const XMLReaderNode& node, bool isAutoCamera);
	virtual ~CameraPoint_PlayerFocus() = default;
	virtual CameraPointType getType(void) const noexcept override { return CameraPointType::PlayerFocus; }
	virtual void getData(const Camera* camera,
					DirectX::XMFLOAT3& position,
					DirectX::XMFLOAT4& rotationQuat) const noexcept override;
	int getNearestCameraDataIndex(const DirectX::XMFLOAT3& position,
								const DirectX::XMFLOAT4& rotationQuat) const noexcept;
	int getNextIndex(int currentIndex, int direction) const noexcept;
private:
	struct Data
	{
		DirectX::XMFLOAT4 _rotationQuat;
		float _distance;
	};
	DirectX::XMFLOAT3 _offset;
	std::vector<Data> _datas;
	bool _isCircular;
};

class CameraPoint_PlayerFocusFixed : public CameraPoint
{
public:
	CameraPoint_PlayerFocusFixed(const XMLReaderNode& node, bool isAutoCamera);
	virtual ~CameraPoint_PlayerFocusFixed() = default;
	virtual CameraPointType getType(void) const noexcept override { return CameraPointType::PlayerFocusFixed; }
	virtual void getData(const Camera* camera,
					DirectX::XMFLOAT3& position,
					DirectX::XMFLOAT4& rotationQuat) const noexcept override;
private:
	DirectX::XMFLOAT3 _offset;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _upVector;
};

class CameraPoint_Path : public CameraPoint
{
public:
	CameraPoint_Path(const XMLReaderNode& node, bool isAutoCamera);
	virtual ~CameraPoint_Path() = default;
	virtual CameraPointType getType(void) const noexcept override { return CameraPointType::Path; }
	virtual void getData(const Camera* camera,
		DirectX::XMFLOAT3& position,
		DirectX::XMFLOAT4& rotationQuat) const noexcept override;
private:
	std::unique_ptr<Path> _path;
};
