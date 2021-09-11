#include "stdafx.h"
#include "Path.h"
#include "FileHelper.h"
#include <xutility>
#include "MathHelper.h"

Path::Path(const XMLReaderNode& node)
{
	using namespace MathHelper;
	node.loadAttribute("IsCurve", _isCurve);
	node.loadAttribute("HasDirection", _hasDirection);
	node.loadAttribute("MoveTick", _moveTick);
	const auto& childNodes = node.getChildNodes();
	_points.reserve(childNodes.size());
	for (const auto& childNode : childNodes)
	{
		_points.emplace_back(childNode, _hasDirection);
	}

	if (_isCurve)
	{
		using namespace DirectX;
		int indexMax = _points.size() / 2;
		for (int i = 0; i < indexMax; i += 2)
		{
			XMVECTOR A = XMLoadFloat3(&_points[i]._position);
			XMVECTOR B = XMLoadFloat3(&_points[i + 2]._position);
			XMVECTOR C = XMLoadFloat3(&_points[i + 1]._position);
			XMVECTOR controlPoint = (2.f * C) - ((A + B) / 2.f);
			XMStoreFloat3(&_points[i + 1]._position, controlPoint);
		}
	}

	// 검증
	if (_moveTick == 0)
	{
		ThrowErrCode(ErrCode::InvalidXmlData);
	}
	if (_points.size() < 2)
	{
		ThrowErrCode(ErrCode::InvalidXmlData);
	}
	if (!equal(_points.front()._time, 0.f) || !equal(_points.back()._time, 1.f))
	{
		ThrowErrCode(ErrCode::InvalidXmlData);
	}
	for (int i = 0; i < _points.size() - 1; ++i)
	{
		if (_points[i + 1]._time <= _points[i]._time)
		{
			ThrowErrCode(ErrCode::InvalidTimeInfo, std::to_string(_points[i]._time));
		}
	}

	if (_isCurve)
	{
		if (_points.size() % 2 != 1)
		{
			ThrowErrCode(ErrCode::InvalidXmlData, "2차 베지어 곡선 사용중");
		}
		if (!_hasDirection)
		{
			for (int i = 0; i < _points.size() - 1; ++i)
			{
				if (equal(_points[i]._position, _points[i + 1]._position))
				{
					ThrowErrCode(ErrCode::InvalidXmlData);
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < _points.size() - 1; ++i)
		{
			const auto& direction = sub(_points[i + 1]._position, _points[i]._position);
			const auto& directionNormal = div(direction, length(direction));
			if (equal(dot(_points[i]._upVector, directionNormal), 1.f))
			{
				ThrowErrCode(ErrCode::InvalidXmlData);
			}
		}
	}
}

void Path::getPathPositionAtTime(const TickCount64& currentMoveTick,
								DirectX::XMFLOAT3& position,
								InterpolationType interpolateType) const noexcept
{
	//float t = (currentMoveTick < _moveTick) ? static_cast<float>(currentMoveTick) / _moveTick : 1.f;
	float t = MathHelper::getInterpolateValue(currentMoveTick, 0, _moveTick, interpolateType);
	if (_isCurve)
	{
		getPathPositionAtTimeCurve(t, position);
	}
	else
	{
		getPathPositionAtTimeLine(t, position);
	}
}

void Path::getPathRotationAtTime(const TickCount64& currentMoveTick,
								DirectX::XMFLOAT4& quaternion,
								InterpolationType interpolateType) const noexcept
{
	//float t = (currentMoveTick < _moveTick) ? static_cast<float>(currentMoveTick) / _moveTick : 1.f;
	float t = MathHelper::getInterpolateValue(currentMoveTick, 0, _moveTick, interpolateType);

	OutputDebugStringA(("t : " + std::to_string(t) + '\n').c_str());
	if (_isCurve)
	{
		getPathRotationAtTimeCurve(t, quaternion);
	}
	else
	{
		getPathRotationAtTimeLine(t, quaternion);
	}
}

const TickCount64& Path::getMoveTick(void) const noexcept
{
	return _moveTick;
}

const DirectX::XMFLOAT3& Path::getPathStartPosition(void) const noexcept
{
	return _points.begin()->_position;
}

void Path::getPathPositionAtTimeCurve(float t, DirectX::XMFLOAT3& position) const noexcept
{
	using namespace DirectX;
	check(0 <= t && t <= 1.f);

	auto it = prev(upper_bound(_points.begin(), _points.end(), t, [](float t, const PathPoint& itor) {
		return t < itor._time; }));
	if (it == _points.end() || next(it) == _points.end())
	{
		it = prev(prev(_points.end()));
	}

	int index = it - _points.begin();
	index -= (index % 2);

	float startT = _points[index]._time;
	float endT = _points[index + 2]._time;
	check(startT <= t && t <= endT);
	float localT = (t - startT) / (endT - startT);
	
	XMVECTOR p0 = XMLoadFloat3(&_points[index]._position);
	XMVECTOR p1 = XMLoadFloat3(&_points[index + 1]._position);
	XMVECTOR p2 = XMLoadFloat3(&_points[index + 2]._position);

	float b0 = (1 - localT) * (1 - localT);
	float b1 = 2 * (1 - localT) * localT;
	float b2 = localT * localT;
	XMVECTOR positionV = (p0 * b0) + (p1 * b1) + (p2 * b2);
	XMStoreFloat3(&position, positionV);
}

void Path::getPathRotationAtTimeCurve(float t, DirectX::XMFLOAT4& quaternion) const noexcept
{
	using namespace DirectX;
	check(0 <= t && t <= 1.f);

	auto it = prev(upper_bound(_points.begin(), _points.end(), t, [](float t, const PathPoint& itor) {
		return t < itor._time; }));
	if (it == _points.end() || next(it) == _points.end())
	{
		it = prev(prev(_points.end()));
	}

	int index = it - _points.begin();
	index -= (index % 2);

	float startT = _points[index]._time;
	float midT = _points[index + 1]._time;
	float endT = _points[index + 2]._time;
	float localT = (t - startT) / (endT - startT);
	float localMidT = (midT - startT) / (endT - startT);

	if (_hasDirection)
	{
		XMVECTOR u0, u1, d0, d1;
		float d;
		if (localT < localMidT)
		{
			u0 = XMLoadFloat3(&_points[index]._upVector);
			u1 = XMLoadFloat3(&_points[index + 1]._upVector);
			d0 = XMLoadFloat3(&_points[index]._direction);
			d1 = XMLoadFloat3(&_points[index + 1]._direction);
			d = localT / localMidT;
		}
		else
		{
			u0 = XMLoadFloat3(&_points[index + 1]._upVector);
			u1 = XMLoadFloat3(&_points[index + 2]._upVector);
			d0 = XMLoadFloat3(&_points[index + 1]._direction);
			d1 = XMLoadFloat3(&_points[index + 2]._direction);
			d = (localT - localMidT) / (1.f - localMidT);
		}
		XMVECTOR r0 = MathHelper::getQuaternion(u0, d0);
		XMVECTOR r1 = MathHelper::getQuaternion(u1, d1);

		XMStoreFloat4(&quaternion, XMQuaternionSlerp(r0, r1, d));
	}
	else
	{
		XMVECTOR p0 = XMLoadFloat3(&_points[index]._position);
		XMVECTOR p1 = XMLoadFloat3(&_points[index + 1]._position);
		XMVECTOR p2 = XMLoadFloat3(&_points[index + 2]._position);

		XMVECTOR direction = XMVector3Normalize(((1 - localT) * (p1 - p0)) + (localT * (p2 - p1)));

		XMVECTOR u0, u1;
		float d;
		if (localT < localMidT)
		{
			u0 = XMLoadFloat3(&_points[index]._upVector);
			u1 = XMLoadFloat3(&_points[index + 1]._upVector);
			d = localT / localMidT;
		}
		else
		{
			u0 = XMLoadFloat3(&_points[index + 1]._upVector);
			u1 = XMLoadFloat3(&_points[index + 2]._upVector);
			d = (localT - localMidT) / (1.f - localMidT);
		}

		XMVECTOR upVector = MathHelper::lerpAxisVector(u0, u1, d);
		XMStoreFloat4(&quaternion, MathHelper::getQuaternion(upVector, direction));
	}
}

void Path::getPathPositionAtTimeLine(float t, DirectX::XMFLOAT3& position) const noexcept
{
	using namespace DirectX;
	check(0 <= t && t <= 1.f);

	auto it0 = prev(upper_bound(_points.begin(), _points.end(), t, [](float t, const PathPoint& itor) {
		return t < itor._time; }));
	if (it0 == _points.end() || next(it0) == _points.end())
	{
		it0 = prev(prev(_points.end()));
	}

	auto it1 = next(it0);
	float localT = (t - it0->_time) / (it1->_time - it0->_time);

	XMVECTOR position0 = XMLoadFloat3(&it0->_position);
	XMVECTOR position1 = XMLoadFloat3(&it1->_position);
	XMStoreFloat3(&position, XMVectorLerp(position0, position1, localT));
}

void Path::getPathRotationAtTimeLine(float t, DirectX::XMFLOAT4& quaternion) const noexcept
{
	using namespace DirectX;
	check(0 <= t && t <= 1.f);

	auto it0 = prev(upper_bound(_points.begin(), _points.end(), t, [](float t, const PathPoint& itor) {
		return t < itor._time; }));
	if (it0 == _points.end() || next(it0) == _points.end())
	{
		it0 = prev(prev(_points.end()));
	}

	auto it1 = next(it0);
	float localT = (t - it0->_time) / (it1->_time - it0->_time);

	XMVECTOR upVector = XMLoadFloat3(&it0->_upVector);
	XMVECTOR direction;
	if (_hasDirection)
	{
		direction = XMLoadFloat3(&it0->_direction);
	}
	else
	{
		XMVECTOR position0 = XMLoadFloat3(&it0->_position);
		XMVECTOR position1 = XMLoadFloat3(&it1->_position);

		direction = XMVector3Normalize(position1 - position0);
	}

	XMStoreFloat4(&quaternion, MathHelper::getQuaternion(upVector, direction));
}

PathPoint::PathPoint(const XMLReaderNode& node, bool hasDirection)
	: _direction(0, 0, 0)
{
	node.loadAttribute("Position", _position);
	node.loadAttribute("UpVector", _upVector);
	XMStoreFloat3(&_upVector, DirectX::XMVector3Normalize(XMLoadFloat3(&_upVector)));
	if (hasDirection)
	{
		node.loadAttribute("Direction", _direction);
		XMStoreFloat3(&_direction, DirectX::XMVector3Normalize(XMLoadFloat3(&_direction)));
	}
	node.loadAttribute("Time", _time);
}
