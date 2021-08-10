#include "stdafx.h"
#include "CollisionHandler.h"
#include "FileHelper.h"
#include "Actor.h"
#include "ActionCondition.h"
#include "ActionChart.h"

CollisionEvent_RotateToTarget::CollisionEvent_RotateToTarget(const XMLReaderNode& node)
{

}

void CollisionEvent_RotateToTarget::process(Actor& selfActor, const Actor&) const noexcept
{
	//selfActor.setRotateType(RotateType::ToCollidingTarget, rotateAngle, )
}

std::unique_ptr<CollisionEvent> CollisionEvent::loadXMLCollisionEvent(const XMLReaderNode& node)
{
	std::string typeString;
	node.loadAttribute("Type", typeString);
	if (typeString == "RotateToTarget")
	{
		return std::make_unique<CollisionEvent_RotateToTarget>(node);
	}
	else if (typeString == "SetAction")
	{
		return std::make_unique<CollisionEvent_SetAction>(node);
	}
	else
	{
		static_assert(static_cast<int>(CollisionEventType::Count) == 2, "Ÿ���߰��� Ȯ���Ұ�");
		ThrowErrCode(ErrCode::UndefinedType, typeString);
	}
}

CollisionEvent_SetAction::CollisionEvent_SetAction(const XMLReaderNode& node)
{
	node.loadAttribute("State", _actionState);
}

void CollisionEvent_SetAction::process(Actor& selfActor, const Actor&) const noexcept
{
	selfActor.setActionState(_actionState);
}

std::string CollisionEvent_SetAction::getActionState(void) const noexcept
{
	return _actionState;
}

CollisionHandler::CollisionHandler(const XMLReaderNode& node)
{
	std::string conditionStrings;
	node.loadAttribute("SelfCondition", conditionStrings);

	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<ActionCondition> condition = ActionCondition::parseConditionString(c);

		if (nullptr != condition) //  [8/10/2021 qwerw] ���� ���Ǹ� ����..
		{
			_selfConditions.push_back(std::move(condition));
		}
	}

	node.loadAttribute("TargetCondition", conditionStrings);

	conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<ActionCondition> condition = ActionCondition::parseConditionString(c);

		if (nullptr != condition) //  [8/10/2021 qwerw] ���� ���Ǹ� ����..
		{
			_targetConditions.push_back(std::move(condition));
		}
	}

	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		_collisionEvents.emplace_back(CollisionEvent::loadXMLCollisionEvent(childNode));
	}
}

void CollisionHandler::checkValid(const ActionChart* actionChart) const
{
	check(actionChart != nullptr);

	for (const auto& event : _collisionEvents)
	{
		// �����Ұ� ���� �� Ÿ�Թۿ� ������ ���� �����Լ��� ������ ���� [8/10/2021 qwerw]
		if (event->getType() == CollisionEventType::SetAction)
		{
			const auto setActionEvent = static_cast<const CollisionEvent_SetAction*>(event.get());
			if (nullptr == actionChart->getActionState(setActionEvent->getActionState()))
			{
				ThrowErrCode(ErrCode::ActionChartLoadFail, setActionEvent->getActionState());
			}
		}
	}
}

bool CollisionHandler::checkHandler(Actor& selfActor, const Actor& targetActor, CollisionCase collisionCase) const noexcept
{
	if (_case == CollisionCase::Lower && collisionCase == CollisionCase::Upper)
	{
		return false;
	}
	else if (_case == CollisionCase::Upper && collisionCase == CollisionCase::Lower)
	{
		return false;
	}

	for (const auto& condition : _selfConditions)
	{
		if (condition->isNotCondition() == condition->checkCondition(selfActor))
		{
			return false;
		}
	}

	for (const auto& condition : _targetConditions)
	{
		if (condition->isNotCondition() == condition->checkCondition(targetActor))
		{
			return false;
		}
	}
	return true;
}
