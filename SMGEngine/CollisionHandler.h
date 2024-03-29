#pragma once
#include "TypeAction.h"

class XMLReaderNode;
class Actor;
class ActionCondition;
class ActionChart;

class CollisionEvent
{
public:
	CollisionEvent() = default;
	virtual ~CollisionEvent() = default;
	virtual void process(Actor& selfActor, const Actor& targetActor) const noexcept = 0;
	virtual CollisionEventType getType(void) const noexcept = 0;
	static std::unique_ptr<CollisionEvent> loadXMLCollisionEvent(const XMLReaderNode& node);
};

class CollisionEvent_RotateToTarget : public CollisionEvent
{
public:
	CollisionEvent_RotateToTarget(const XMLReaderNode& node);
	virtual ~CollisionEvent_RotateToTarget() = default;
	virtual void process(Actor& selfActor, const Actor& targetActor) const noexcept override;
	virtual CollisionEventType getType(void) const noexcept { return CollisionEventType::RotateToTarget; }
private:
	float _speed;
};

class CollisionEvent_SetAction : public CollisionEvent
{
public:
	CollisionEvent_SetAction(const XMLReaderNode& node);
	virtual ~CollisionEvent_SetAction() = default;
	virtual void process(Actor& selfActor, const Actor& targetActor) const noexcept override;
	virtual CollisionEventType getType(void) const noexcept { return CollisionEventType::SetAction; }
	std::string getActionState(void) const noexcept;
private:
	std::string _actionState;
};

class CollisionEvent_SetVariableFromTarget : public CollisionEvent
{
public:
	CollisionEvent_SetVariableFromTarget(const XMLReaderNode& node);
	virtual ~CollisionEvent_SetVariableFromTarget() = default;
	virtual void process(Actor& selfActor, const Actor& targetActor) const noexcept override;
	virtual CollisionEventType getType(void) const noexcept { return CollisionEventType::SetVariableFromTarget; }
private:
	std::string _name;
};

class CollisionHandler
{
public:
	CollisionHandler(const XMLReaderNode& node);
	~CollisionHandler() = default;
	void checkValid(const ActionChart* actionChart) const;
	bool checkHandler(Actor& selfActor, const Actor& targetActor, CollisionCase collisionCase) const noexcept;
	void processEvents(Actor& selfActor, const Actor& targetActor) const noexcept;
private:
	CollisionCase _case;
	std::vector<std::unique_ptr<CollisionEvent>> _collisionEvents;
	std::vector<std::unique_ptr<ActionCondition>> _selfConditions;
	std::vector<std::unique_ptr<ActionCondition>> _targetConditions;
};