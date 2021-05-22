#include "stdafx.h"
#include "ActionCondition.h"
#include "Actor.h"
#include "Exception.h"
#include "SMGFramework.h"
#include "D3DUtil.h"

ActionCondition_Tick::ActionCondition_Tick(const std::string& args)
{
	const auto& tokenized = D3DUtil::tokenizeString(args, '_');
	if (tokenized.size() != 2)
	{
		ThrowErrCode(ErrCode::ActionChartLoadFail, "ActionCondition_Tick" + args);
	}
	_tickStart = std::stoi(tokenized[0]);
	_tickEnd = std::stoi(tokenized[1]);
}

bool ActionCondition_Tick::checkCondition(const Actor& actor) const noexcept
{
	uint32_t current = actor.getLocalTickCount();
	if (_tickStart <= current && current < _tickEnd)
	{
		return true;
	}
	return false;
}

bool ActionCondition_End::checkCondition(const Actor& actor) const noexcept
{
	if (actor.isActionEnd())
	{
		return true;
	}
	return false;
}

ActionCondition_Key::ActionCondition_Key(const std::string& args)
{
	 const auto& tokenized = D3DUtil::tokenizeString(args, '_');
	 if (tokenized.size() != 2)
	 {
		 ThrowErrCode(ErrCode::ActionChartLoadFail, "ActionCondition_Key" + args);
	 }

	 if (tokenized[0] == "Down")
	 {
		 _buttonState = ButtonState::Down;
	 }
	 else if (tokenized[0] == "Press")
	 {
		 _buttonState = ButtonState::Press;
	 }
	 else if (tokenized[0] == "Up")
	 {
		 _buttonState = ButtonState::Up;
	 }
	 else if (tokenized[0] == "None")
	 {
		 _buttonState = ButtonState::None;
	 }
	 else
	 {
		 static_assert(static_cast<int>(ButtonState::None) == 3, "타입 추가시 확인");
		 ThrowErrCode(ErrCode::ActionChartLoadFail, tokenized[0]);
	 }

	 if (tokenized[1] == "AB")
	 {
		 _buttonType = ButtonInputType::AB;
	 }
	 else if (tokenized[1] == "XY")
	 {
		 _buttonType = ButtonInputType::XY;
	 }
	 else if (tokenized[1] == "ZL")
	 {
		 _buttonType = ButtonInputType::ZL;
	 }
	 else if (tokenized[1] == "ZR")
	 {
		 _buttonType = ButtonInputType::ZR;
	 }
	 else if (tokenized[1] == "L")
	 {
		 _buttonType = ButtonInputType::L;
	 }
	 else if (tokenized[1] == "R")
	 {
		 _buttonType = ButtonInputType::R;
	 }
	 else if (tokenized[1] == "LStickButton")
	 {
		 _buttonType = ButtonInputType::LStickButton;
	 }
	 else if (tokenized[1] == "RStickButton")
	 {
		 _buttonType = ButtonInputType::RStickButton;
	 }
	 else
	 {
		 static_assert(static_cast<int>(ButtonInputType::Count) == 8, "타입 추가시 확인");
		 ThrowErrCode(ErrCode::ActionChartLoadFail, tokenized[1]);
	 }
}

bool ActionCondition_Key::checkCondition(const Actor& actor) const noexcept
{
	if (_buttonState == SMGFramework::Get().getButtonInput(_buttonType))
	{
		return true;
	}

	return false;
}

ActionCondition::ActionCondition()
	: _not(false)
{

}

std::unique_ptr<ActionCondition> ActionCondition::parseConditionString(const std::string& input)
{
	bool notCondition;
	std::string conditionString;
	if (input.at(0) == '!')
	{
		conditionString = input.substr(1);
		notCondition = true;
	}
	else
	{
		conditionString = input;
		notCondition = false;
	}
	std::unique_ptr<ActionCondition> condition = nullptr;

	size_t cursor = std::min(conditionString.find('_'), conditionString.length());
	std::string conditionTypeString = conditionString.substr(0, cursor);
	std::string conditionArgs = (cursor < conditionString.length()) ? conditionString.substr(cursor + 1) : "";

	// 길어지면 switch case로.. [3/25/2021 qwerwy]
	if (conditionTypeString == "Tick")
	{
		condition = std::make_unique<ActionCondition_Tick>(conditionArgs);
	}
	else if (conditionTypeString == "End")
	{
		condition = std::make_unique<ActionCondition_End>();
	}
	else if (conditionTypeString == "Key")
	{
		condition = std::make_unique<ActionCondition_Key>(conditionArgs);
	}
	else if (conditionTypeString == "LStickMove")
	{
		return nullptr;
	}
	else if (conditionTypeString == "RStickMove")
	{
		return nullptr;
	}
	else if (conditionTypeString == "OnGround")
	{
		return nullptr;
	}
	else if (conditionTypeString == "GroundEdge")
	{
		return nullptr;
	}
	else
	{
		static_assert(static_cast<int>(ActionConditionType::Count) == 3, "타입이 추가되면 작업되어야 합니다.");
		ThrowErrCode(ErrCode::UndefinedType, "conditionString : " + conditionString);
	}

	if (notCondition)
	{
		condition->setNotCondition();
	}
	return condition;
}
