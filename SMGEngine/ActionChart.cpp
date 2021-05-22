#include "stdafx.h"
#include "ActionChart.h"
#include "Actor.h"
#include "Exception.h"
#include "FileHelper.h"
#include "D3DUtil.h"
#include "TypeAction.h"
#include "ActionCondition.h"
#include "FrameEvent.h"

ActionBranch::ActionBranch(const XMLReaderNode& node)
{
	node.loadAttribute("State", _actionState);
	std::string conditionStrings;
	node.loadAttribute("Condition", conditionStrings);
	
	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<ActionCondition> condition = ActionCondition::parseConditionString(c);
		
		if (nullptr != condition) // 개발 편의를 위해..
		{
			_actionConditions.push_back(std::move(condition));
		}
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
		_actionStates.emplace(childNode.getNodeName(), new ActionState(childNode));
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

ActionState::ActionState(const XMLReaderNode& node)
{
	node.loadAttribute("Animation", _animationName);
	node.loadAttribute("BlendTick", _blendTick);

	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		const std::string& nodeName = childNode.getNodeName();
		if (nodeName == "Branch")
		{
			_branches.emplace_back(new ActionBranch(childNode));
		}
		else if(nodeName == "FrameEvent")
		{
			//_frameEvents.emplace_back(childNode);
		}
		else
		{
			ThrowErrCode(ErrCode::InvalidXmlData, nodeName);
		}
	}
}

bool ActionState::checkBranch(Actor& actor, std::string& nextState) const noexcept
{
	for (const auto& branch : _branches)
	{
		if (branch->checkBranchCondition(actor))
		{
			nextState = branch->getActionState();
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
