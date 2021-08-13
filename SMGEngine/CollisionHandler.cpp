#include "stdafx.h"
#include "CollisionHandler.h"
#include "FileHelper.h"
#include "Actor.h"
#include "ActionCondition.h"
#include "ActionChart.h"
#include "MathHelper.h"

CollisionEvent_RotateToTarget::CollisionEvent_RotateToTarget(const XMLReaderNode& node)
{
	node.loadAttribute("Speed", _speed);
}

void CollisionEvent_RotateToTarget::process(Actor& selfActor, const Actor& targetActor) const noexcept
{
	using namespace DirectX;
	XMVECTOR selfUpVector = XMLoadFloat3(&selfActor.getUpVector());
	XMVECTOR selfDirection = XMLoadFloat3(&selfActor.getDirection());
	XMVECTOR toTarget = XMLoadFloat3(&targetActor.getPosition()) - XMLoadFloat3(&selfActor.getPosition());
	float deltaAngle = MathHelper::getDeltaAngleToVector(selfUpVector, selfDirection, toTarget);

	selfActor.setRotateType(RotateType::ToCollidingTarget, deltaAngle, _speed);
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
		static_assert(static_cast<int>(CollisionEventType::Count) == 2, "타입추가시 확인할것");
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
	std::string caseString;
	node.loadAttribute("Case", caseString);
	if (caseString == "Upper")
	{
		_case = CollisionCase::Upper;
	}
	else if (caseString == "Center")
	{
		_case = CollisionCase::Center;
	}
	else if (caseString == "Lower")
	{
		_case = CollisionCase::Lower;
	}
	else
	{
		static_assert(static_cast<int>(CollisionCase::All) == 3);
		ThrowErrCode(ErrCode::UndefinedType, caseString);
	}
	std::string conditionStrings;
	node.loadAttribute("SelfCondition", conditionStrings);

	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<ActionCondition> condition = ActionCondition::parseConditionString(c);

		if (nullptr != condition) //  [8/10/2021 qwerw] 개발 편의를 위해..
		{
			_selfConditions.push_back(std::move(condition));
		}
	}

	node.loadAttribute("TargetCondition", conditionStrings);

	conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<ActionCondition> condition = ActionCondition::parseConditionString(c);

		if (nullptr != condition) //  [8/10/2021 qwerw] 개발 편의를 위해..
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
		// 검증할게 아직 이 타입밖에 없으니 굳이 가상함수를 만들진 않음 [8/10/2021 qwerw]
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
	if (_case != CollisionCase::All && _case != collisionCase)
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

void CollisionHandler::processEvents(Actor& selfActor, const Actor& targetActor) const noexcept
{
	for (const auto& collisionEvent : _collisionEvents)
	{
		collisionEvent->process(selfActor, targetActor);
	}
}
