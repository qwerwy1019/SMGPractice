#include "stdafx.h"
#include "ActionChart.h"
#include "Actor.h"
#include "Exception.h"
#include "FileHelper.h"
#include "D3DUtil.h"
#include "TypeAction.h"
#include "ActionCondition.h"
#include "FrameEvent.h"

void ActionBranch::loadXML(const XMLReaderNode& node)
{
	node.loadAttribute("Name", _actionState);
	std::string conditionStrings;
	node.loadAttribute("Condition", conditionStrings);
	
	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		bool notCondition;
		std::string conditionString;
		if (c.at(0) == '!')
		{
			conditionString = c.substr(1);
			notCondition = true;
		}
		else
		{
			conditionString = c;
			notCondition = false;
		}
		std::unique_ptr<ActionCondition> condition = nullptr;

		size_t cursor = std::min(conditionString.find('_'), conditionString.length());
		std::string conditionTypeString = conditionString.substr(0, cursor);
		std::string conditionParameterString = conditionString.substr(cursor + 1);

		// 길어지면 switch case로.. [3/25/2021 qwerwy]
		if (conditionString == "Tick")
		{
			condition = std::make_unique<ActionCondition_Tick>();
			condition->loadXML(conditionParameterString);
		}
		else if (conditionString == "End")
		{
			condition = std::make_unique<ActionCondition_End>();
			condition->loadXML(conditionParameterString);
		}
		else if (conditionString == "Button")
		{
			condition = std::make_unique<ActionCondition_Button>();
			condition->loadXML(conditionParameterString);
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

		_actionConditions.push_back(std::move(condition));
	}
}

bool ActionBranch::checkBranchCondition(const Actor& actor) const noexcept
{
	for (const auto& condition : _actionConditions)
	{
		if (false == condition->checkCondition(actor))
		{
			return false;
		}
	}
	return true;
}

ActionChart::ActionChart(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		
	}
}

ActionState* ActionChart::getActionState(const std::string& name) const noexcept
{
	auto it = _actionStates.find(name);
	if (it == _actionStates.end())
	{
		check(false, name + " ActionState를 찾을수 없습니다.");
		return nullptr;
	}
	return it->second.get();
}

// void ActionChart::updateActionState(Actor& actor, uint32_t deltaFrame) const noexcept
// {
// 	std::string nextStateName;
// 	if (actor.getCurrentActionState()->checkBranch(actor, nextStateName))
// 	{
// 		ActionState* nextState = getActionState(nextStateName);
// 		actor.setActionState(nextState);
// 	}
// }

bool ActionState::checkBranch(Actor& actor, std::string& nextState) const noexcept
{
	for (const auto& branch : _branches)
	{
		if (branch.checkBranchCondition(actor))
		{
			nextState = branch.getActionState();
			return true;
		}
	}
	return false;
}

void ActionState::processFrameEvents(Actor& actor, const TickCount64& lastProcessedTick, const TickCount64& progressedTick) const noexcept
{
	// 바이너리서치로 변경할것 [4/29/2021 qwerwy]
	for (const auto& frameEvent : _frameEvents)
	{
		TickCount64 tick = frameEvent->getProcessTick();
		if (lastProcessedTick < tick)
		{
			if (progressedTick < tick)
			{
				break;
			}
			if (frameEvent->checkConditions(actor))
			{
				frameEvent->process(actor);
			}
		}
	}
}

std::string ActionState::getAnimationName(void) const noexcept
{
	return _animationName;
}

TickCount64 ActionState::getBlendTick(void) const noexcept
{
	return _blendTick;
}
