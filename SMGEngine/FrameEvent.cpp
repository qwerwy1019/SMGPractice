#include "stdafx.h"
#include "FrameEvent.h"
#include "Actor.h"
#include "ActionCondition.h"
#include "Exception.h"
#include "MathHelper.h"

FrameEvent::FrameEvent()
	: _processTick(0)
{
	
}

bool FrameEvent::checkConditions(Actor& actor) const noexcept
{
	for (const auto& condition : _conditions)
	{
		if (!condition->checkCondition(actor))
		{
			return false;
		}
	}
	return true;
}

FrameEvent_RotateType::FrameEvent_RotateType()
	: _rotateType(RotateType::Count)
	, _offset(0)
	, _rotateSpeed(0)
{

}

void FrameEvent_RotateType::process(Actor& actor) const noexcept
{
	check(0 <= _offset && _offset < MathHelper::Pi);
	actor.setRotateType(_rotateType, _offset, _rotateSpeed);
}

FrameEvent_SpeedUp::FrameEvent_SpeedUp()
	: _maxSpeed(0)
	, _acceleration(0)
	, _moveType(MoveType::Count)
{
	
}

void FrameEvent_SpeedUp::process(Actor& actor) const noexcept
{
	check(_acceleration > 0.f);
	check(_maxSpeed > 0.f);
	actor.setAcceleration(_acceleration, _maxSpeed);
}

FrameEvent_SpeedDown::FrameEvent_SpeedDown()
	: _minSpeed(0)
	, _deceleration(0)
{

}

void FrameEvent_SpeedDown::process(Actor& actor) const noexcept
{
	check(_deceleration > 0.f);
	check(_minSpeed >= 0.f);
	actor.setDeceleration(_deceleration, _minSpeed);
}

FrameEvent_Jump::FrameEvent_Jump()
	: _speed(0)
{

}

void FrameEvent_Jump::process(Actor& actor) const noexcept
{
	actor.setVerticalSpeed(_speed);
}