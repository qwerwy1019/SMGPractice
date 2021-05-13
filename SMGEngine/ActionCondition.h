#pragma once
#include "TypeAction.h"
#include "TypeCommon.h"

class Actor;

class ActionCondition
{
public:
	virtual ActionConditionType getType(void) const noexcept = 0;
	virtual void loadXML(const std::string& condition) = 0;
	virtual bool checkCondition(const Actor& actor) const noexcept = 0;
	void setNotCondition(void) noexcept { _not = true; }
public:
	bool _not;
};
class ActionCondition_Tick : public ActionCondition
{
public:
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Tick; }
	virtual void loadXML(const std::string& condition) override;
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	uint32_t _frameStart;
	uint32_t _frameEnd;
};

class ActionCondition_Button : public ActionCondition
{
public:
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Button; }
	virtual void loadXML(const std::string& condition) override;
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	ButtonInputType _buttonType;
	ButtonState _buttonState;
};

class ActionCondition_End : public ActionCondition
{
public:
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::End; }
	virtual void loadXML(const std::string& condition) override { }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
};
