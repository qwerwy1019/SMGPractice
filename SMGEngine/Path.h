#pragma once
#include "TypeCommon.h"
class XMLReaderNode;

struct PathPoint
{
	PathPoint(const XMLReaderNode& node, bool hasDirection);
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _upVector;
	DirectX::XMFLOAT3 _direction;
	float _time;
};
class Path
{
public:
	Path(const XMLReaderNode& node);
	void getPathPositionAtTime(const TickCount64& currentMoveTick,
		DirectX::XMFLOAT3& position, InterpolationType interpolateType) const noexcept;
	void getPathRotationAtTime(const TickCount64& currentMoveTick,
		DirectX::XMFLOAT4& quaternion, InterpolationType interpolateType) const noexcept;
	const TickCount64& getMoveTick(void) const noexcept;
	const DirectX::XMFLOAT3& getPathStartPosition(void) const noexcept;
private:
	void getPathPositionAtTimeCurve(float t,
		DirectX::XMFLOAT3& position) const noexcept;
	void getPathRotationAtTimeCurve(float t,
		DirectX::XMFLOAT4& quaternion) const noexcept;
	void getPathPositionAtTimeLine(float t,
		DirectX::XMFLOAT3& position) const noexcept;
	void getPathRotationAtTimeLine(float t,
		DirectX::XMFLOAT4& quaternion) const noexcept;
private:
	std::vector<PathPoint> _points;
	TickCount64 _moveTick;
	bool _isCurve;
	bool _hasDirection;
};