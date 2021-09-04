#include "stdafx.h"
#include "ActionChart.h"
#include "Actor.h"
#include "Exception.h"
#include "FileHelper.h"
#include "D3DUtil.h"
#include "TypeAction.h"
#include "ActionCondition.h"
#include "FrameEvent.h"
#include "CollisionHandler.h"

ActionBranch::ActionBranch(const XMLReaderNode& node)
{
	node.loadAttribute("State", _actionState);
	std::string conditionStrings;
	node.loadAttribute("Condition", conditionStrings);
	
	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	_actionConditions.reserve(conditionTokenized.size());
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
		if (condition->isNotCondition() == condition->checkCondition(actor))
		{
			return false;
		}
	}
	return true;
}

ActionChart::ActionChart(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodesWithName();
	auto childIter = childNodes.find("ActionStates");
	const auto& actionStates = childIter->second.getChildNodes();
	for (const auto& actionState : actionStates)
	{
		auto it = _actionStates.emplace(actionState.getNodeName(), std::make_unique<ActionState>(actionState));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::ActionChartLoadFail, actionState.getNodeName());
		}
	}
	childIter = childNodes.find("CollisionHandlers");
	const auto& collisionHandlers = childIter->second.getChildNodes();
	for (const auto& collisionHandler : collisionHandlers)
	{
		_collisionHandlers.emplace_back(std::make_unique<CollisionHandler>(collisionHandler));
	}
	childIter = childNodes.find("Variables");
	const auto& variables = childIter->second.getChildNodes();
	for (const auto& variable : variables)
	{
		std::string name;
		variable.loadAttribute("Name", name);
		int value;
		variable.loadAttribute("Value", value);
		auto it = _variables.emplace(name, value);
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::ActionChartLoadFail, name);
		}
	}
	checkValid();
}

ActionChart::~ActionChart()
{

}

ActionState* ActionChart::getActionState(const std::string& name) const noexcept
{
	auto it = _actionStates.find(name);
	if (it == _actionStates.end())
	{
		return nullptr;
	}
	return it->second.get();
}

void ActionChart::checkValid(void) const
{
	auto it = _actionStates.find("IDLE");
	if (it == _actionStates.end())
	{
		ThrowErrCode(ErrCode::ActionChartLoadFail, "IDLE 액션이 없습니다.");
	}
	for (const auto& actionState : _actionStates)
	{
		actionState.second->checkValid(this);
	}
	for (const auto& collisionHandler : _collisionHandlers)
	{
		collisionHandler->checkValid(this);
	}
}

void ActionChart::processCollisionHandlers(Actor& selfActor, const Actor& targetActor, CollisionCase collisionCase) const noexcept
{
	for (const auto& collisionHandler : _collisionHandlers)
	{
		if (collisionHandler->checkHandler(selfActor, targetActor, collisionCase))
		{
			collisionHandler->processEvents(selfActor, targetActor);
			break;
		}
	}
}

const std::unordered_map<std::string, int>& ActionChart::getVariables(void) const noexcept
{
	return _variables;
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
			_branches.emplace_back(childNode);
		}
		else if(nodeName == "FrameEvent")
		{
			_frameEvents.emplace_back(FrameEvent::loadXMLFrameEvent(childNode));
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
	check(lastProcessedTick < progressedTick);
	// 바이너리서치로 변경할것 [4/29/2021 qwerwy]
	for (const auto& frameEvent : _frameEvents)
	{
		TickCount64 tick = frameEvent->getProcessTick();
		if (lastProcessedTick < tick)
		{
			if (progressedTick < tick)
			{
				continue;
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

void ActionState::checkValid(const ActionChart* actionChart) const
{
	check(actionChart != nullptr);

	for (const auto& branch : _branches)
	{
		if (nullptr == actionChart->getActionState(branch.getActionState()))
		{
			ThrowErrCode(ErrCode::ActionChartLoadFail, branch.getActionState());
		}
	}
}

