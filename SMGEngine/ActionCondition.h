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
