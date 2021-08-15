#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"
class Actor;
class ActionCondition;
class XMLReaderNode;

class FrameEvent
{
public:
	FrameEvent(const XMLReaderNode& node);
	virtual ~FrameEvent() = default;
	virtual void process(Actor& actor) const noexcept = 0;
	virtual FrameEventType getType() const noexcept = 0;
	TickCount64 getProcessTick(void) const noexcept { return _processTick; }
	bool checkConditions(Actor& actor) const noexcept;
	static std::unique_ptr<FrameEvent> loadXMLFrameEvent(const XMLReaderNode& node);
private:
	TickCount64 _processTick;
	std::vector<std::unique_ptr<ActionCondition>> _conditions;
};

class FrameEvent_Rotate : public FrameEvent
{
public:
	FrameEvent_Rotate(const XMLReaderNode& node);
	virtual ~FrameEvent_Rotate() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Rotate; }
private:
	RotateType _rotateType;
	float _offset;
	float _rotateSpeed;
};

class FrameEvent_Speed : public FrameEvent
{
public:
	FrameEvent_Speed(const XMLReaderNode& node);
	virtual ~FrameEvent_Speed() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Speed; }
	static constexpr float ACCELERATION_UNIT = 0.001f;
private:
	float _targetSpeed;
	float _acceleration;
	MoveType _moveType;
};

class FrameEvent_Jump : public FrameEvent
{
public:
	FrameEvent_Jump(const XMLReaderNode& node);
	virtual ~FrameEvent_Jump() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Jump; }
private:
	float _speed;
	float _targetFallSpeed;
};

class FrameEvent_Die : public FrameEvent
{
public:
	FrameEvent_Die(const XMLReaderNode& node);
	virtual ~FrameEvent_Die() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Die; }
private:
};

class FrameEvent_SpawnCharacter : public FrameEvent
{
public:
	FrameEvent_SpawnCharacter(const XMLReaderNode& node);
	virtual ~FrameEvent_SpawnCharacter() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::SpawnCharacter; }
private:
	CharacterKey _characterKey;
	DirectX::XMFLOAT3 _position;
	float _size;
};