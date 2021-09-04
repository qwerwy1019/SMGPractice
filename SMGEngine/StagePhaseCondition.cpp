#include "stdafx.h"
#include "StagePhaseCondition.h"
#include "StageManager.h"
#include "Exception.h"
#include "D3DUtil.h"
#include "SMGFramework.h"
#include "UserData.h"

StagePhaseCondition::StagePhaseCondition()
	: _not(false)
{

}

std::unique_ptr<StagePhaseCondition> StagePhaseCondition::parseConditionString(const std::string& input)
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
	std::unique_ptr<StagePhaseCondition> condition = nullptr;

	size_t cursor = std::min(conditionString.find('_'), conditionString.length());
	std::string conditionTypeString = conditionString.substr(0, cursor);
	std::string conditionArgs = (cursor < conditionString.length()) ? conditionString.substr(cursor + 1) : "";

	// 길어지면 switch case로.. [9/3/2021 qwerwy]
	if (conditionTypeString == "Variable")
	{
		condition = std::make_unique<StagePhaseCondition_Variable>(conditionArgs);
	}
	else if (conditionTypeString == "UserData")
	{
		condition = std::make_unique<StagePhaseCondition_UserData>(conditionArgs);
	}
	else
	{
		static_assert(static_cast<int>(StagePhaseConditionType::Count) == 2, "타입이 추가되면 작업되어야 합니다.");
		ThrowErrCode(ErrCode::UndefinedType, "conditionString : " + conditionString);
	}

	if (notCondition)
	{
		condition->setNotCondition();
	}
	return condition;
}

StagePhaseCondition_Variable::StagePhaseCondition_Variable(const std::string& args)
{
	bool rv = D3DUtil::parseComparison(args, _name, _operator, _value);
	if (!rv)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "StagePhaseCondition_Variable" + args);
	}
}

bool StagePhaseCondition_Variable::checkCondition(void) const noexcept
{
	check(SMGFramework::getStageManager() != nullptr);
	int variable = SMGFramework::getStageManager()->getStageScriptVariable(_name);

	return D3DUtil::compare(_operator, variable, _value);
}

StagePhaseCondition_UserData::StagePhaseCondition_UserData(const std::string& args)
{
	// save file 작업하면서 추후 수정 [9/3/2021 qwerw]
	const auto& tokenized = D3DUtil::tokenizeString(args, '_');
	if (tokenized.size() != 2)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "StagePhaseCondition_UserData" + args);
	}
	_name = tokenized[0];
	_value = std::stoi(tokenized[1]);
}

bool StagePhaseCondition_UserData::checkCondition(void) const noexcept
{
	// save file 작업하면서 추후 수정 [9/3/2021 qwerw]
	if (_name == "Life")
	{
		if (SMGFramework::Get().getUserData()->getLife() == _value)
		{
			return true;
		}
		return false;
	}
	check(false, "미구현");
	return false;
}
