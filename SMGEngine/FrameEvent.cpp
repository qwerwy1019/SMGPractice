#include "stdafx.h"
#include "FrameEvent.h"
#include "Actor.h"
#include "ActionCondition.h"
#include "Exception.h"
#include "MathHelper.h"
#include "FileHelper.h"

FrameEvent::FrameEvent(const XMLReaderNode& node)
{
	node.loadAttribute("Tick", _processTick);
	
	std::string conditionStrings;
	node.loadAttribute("Condition", conditionStrings);

	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<ActionCondition> condition = ActionCondition::parseConditionString(c);

		if (nullptr != condition) // ���� ���Ǹ� ����..
		{
			_conditions.push_back(std::move(condition));
		}
	}
}

bool FrameEvent::checkConditions(Actor& actor) const noexcept
{
	for (const auto& condition : _conditions)
	{
		if (condition->isNotCondition() == condition->checkCondition(actor))
		{
			return false;
		}
	}
	return true;
}

std::unique_ptr<FrameEvent> FrameEvent::loadXMLFrameEvent(const XMLReaderNode& node)
{
	std::string typeString;
	node.loadAttribute("Type", typeString);
	if (typeString == "Rotate")
	{
		return std::make_unique<FrameEvent_Rotate>(node);
	}
	else if (typeString == "Speed")
	{
		return std::make_unique<FrameEvent_Speed>(node);
	}
	else if (typeString == "Jump")
	{
		return std::make_unique<FrameEvent_Jump>(node);
	}
	else
	{
		static_assert(static_cast<int>(FrameEventType::Count) == 3, "Ÿ���߰��� Ȯ���Ұ�");
		ThrowErrCode(ErrCode::UndefinedType, typeString);
	}
}

FrameEvent_Rotate::FrameEvent_Rotate(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("Offset", _offset);
	node.loadAttribute("Speed", _rotateSpeed);

	std::string typeString;
	node.loadAttribute("RotateType", typeString);
	if (typeString == "ToTarget")
	{
		_rotateType = RotateType::ToTarget;
	}
	else if (typeString == "Path")
	{
		_rotateType = RotateType::Path;
	}
	else if (typeString == "Input")
	{
		_rotateType = RotateType::Input;
	}
	else
	{
		static_assert(static_cast<int>(RotateType::Count) == 3);
		ThrowErrCode(ErrCode::UndefinedType, typeString);
	}
}

void FrameEvent_Rotate::process(Actor& actor) const noexcept
{
	check(0 <= _offset && _offset < MathHelper::Pi);
	actor.setRotateType(_rotateType, _offset, _rotateSpeed);
}

FrameEvent_Speed::FrameEvent_Speed(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("TargetSpeed", _targetSpeed);
	node.loadAttribute("Acceleration", _acceleration);
	std::string typeString;
	node.loadAttribute("MoveType", typeString);
	if (typeString == "Fixed")
	{
		_moveType = MoveType::Fixed;
	}
	else if (typeString == "CharacterDirection")
	{
		_moveType = MoveType::CharacterDirection;
	}
	else if (typeString == "JoystickDirection")
	{
		_moveType = MoveType::JoystickDirection;
	}
	else if (typeString == "Path")
	{
		_moveType = MoveType::Path;
	}
	else
	{
		static_assert(static_cast<int>(MoveType::Count), "Ÿ�� �߰��� Ȯ��");
		ThrowErrCode(ErrCode::UndefinedType, typeString);
	}
}

void FrameEvent_Speed::process(Actor& actor) const noexcept
{
	check(_acceleration > 0.f);
	check(_targetSpeed >= 0.f);
	actor.setAcceleration(_acceleration, _targetSpeed);
}

FrameEvent_Jump::FrameEvent_Jump(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("Speed", _speed);
}

void FrameEvent_Jump::process(Actor& actor) const noexcept
{
	actor.setVerticalSpeed(_speed);
}