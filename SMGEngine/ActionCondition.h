#pragma once
#include "TypeAction.h"
#include "TypeCommon.h"

class Actor;

class ActionCondition
{
public:
	ActionCondition();
	virtual ~ActionCondition() = default;
	virtual ActionConditionType getType(void) const noexcept = 0;
	virtual bool checkCondition(const Actor& actor) const noexcept = 0;
	bool isNotCondition(void) const noexcept { return _not; }
	void setNotCondition(void) noexcept { _not = true; }
	static std::unique_ptr<ActionCondition> parseConditionString(const std::string& input);
public:
	bool _not;
};
class ActionCondition_Tick : public ActionCondition
{
public:
	ActionCondition_Tick(const std::string& args);
	virtual ~ActionCondition_Tick() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Tick; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	uint32_t _tickStart;
	uint32_t _tickEnd;
};

class ActionCondition_Key : public ActionCondition
{
public:
	ActionCondition_Key(const std::string& args);
	virtual ~ActionCondition_Key() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Key; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	ButtonInputType _buttonType;
	ButtonState _buttonState;
};

class ActionCondition_End : public ActionCondition
{
public:
	ActionCondition_End() = default;
	virtual ~ActionCondition_End() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::End; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
};

class ActionCondition_Stick : public ActionCondition
{
public:
	ActionCondition_Stick(const std::string& args);
	virtual ~ActionCondition_Stick() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Key; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	StickInputType _stickType;
	StickInputState _stickInputState;
};

class ActionCondition_IsStop : public ActionCondition
{
public:
	ActionCondition_IsStop(const std::string& args);
	virtual ~ActionCondition_IsStop() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::IsStop; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_OnGround : public ActionCondition
{
public:
	ActionCondition_OnGround(const std::string& args);
	virtual ~ActionCondition_OnGround() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::OnGround; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};