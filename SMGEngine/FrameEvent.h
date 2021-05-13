#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"
class Actor;
class ActionCondition;

class FrameEvent
{
public:
	FrameEvent();
	virtual void process(Actor& actor) const noexcept = 0;
	TickCount64 getProcessTick(void) const noexcept { return _processTick; }
	bool checkConditions(Actor& actor) const noexcept;
private:
	TickCount64 _processTick;
	std::vector<std::unique_ptr<ActionCondition>> _conditions;
};

class FrameEvent_RotateType : public FrameEvent
{
public:
	FrameEvent_RotateType();
	virtual void process(Actor& actor) const noexcept override;
private:
	RotateType _rotateType;
	float _offset;
	float _rotateSpeed;
};

class FrameEvent_SpeedUp : public FrameEvent
{
public:
	FrameEvent_SpeedUp();
	virtual void process(Actor& actor) const noexcept override;
private:
	float _maxSpeed;
	float _acceleration;
	MoveType _moveType;
};

class FrameEvent_SpeedDown : public FrameEvent
{
public:
	FrameEvent_SpeedDown();
	virtual void process(Actor& actor) const noexcept override;
private:
	float _minSpeed;
	float _deceleration;
};

class FrameEvent_Jump : public FrameEvent
{
public:
	FrameEvent_Jump();
	virtual void process(Actor& actor) const noexcept override;
private:
	float _speed;
};