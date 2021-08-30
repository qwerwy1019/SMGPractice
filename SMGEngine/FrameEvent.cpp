#include "stdafx.h"
#include "FrameEvent.h"
#include "Actor.h"
#include "ActionCondition.h"
#include "Exception.h"
#include "MathHelper.h"
#include "FileHelper.h"
#include "SMGFramework.h"
#include "StageManager.h"
#include "CharacterInfoManager.h"
#include "StageInfo.h"
#include "D3DApp.h"
#include "GameObject.h"
#include "Effect.h"
#include "Camera.h"

FrameEvent::FrameEvent(const XMLReaderNode& node)
{
	node.loadAttribute("Tick", _processTick);
	
	std::string conditionStrings;
	node.loadAttribute("Condition", conditionStrings);

	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<ActionCondition> condition = ActionCondition::parseConditionString(c);

		if (nullptr != condition) // 개발 편의를 위해..
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
	else if (typeString == "Die")
	{
		return std::make_unique<FrameEvent_Die>(node);
	}
	else if (typeString == "SpawnCharacter")
	{
		return std::make_unique<FrameEvent_SpawnCharacter>(node);
	}
	else if (typeString == "Effect")
	{
		return std::make_unique<FrameEvent_Effect>(node);
	}
	else if (typeString == "SetVariable")
	{
		return std::make_unique<FrameEvent_SetVariable>(node);
	}
	else if (typeString == "FallSpeed")
	{
		return std::make_unique<FrameEvent_FallSpeed>(node);
	}
	else if (typeString == "SetPath")
	{
		return std::make_unique<FrameEvent_SetPath>(node);
	}
	else if (typeString == "Gravity")
	{
		return std::make_unique<FrameEvent_Gravity>(node);
	}
	else if (typeString == "SetCamera")
	{
		return std::make_unique<FrameEvent_SetCamera>(node);
	}
	else if (typeString == "Collision")
	{
		return std::make_unique<FrameEvent_Collision>(node);
	}
	else
	{
		static_assert(static_cast<int>(FrameEventType::Count) == 12, "타입추가시 확인할것");
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
	if (typeString == "Fixed")
	{
		_rotateType = RotateType::Fixed;
	}
	else if (typeString == "ToPlayer")
	{
		_rotateType = RotateType::ToPlayer;
	}
	else if (typeString == "Path")
	{
		_rotateType = RotateType::Path;
	}
	else if (typeString == "JoystickInput")
	{
		_rotateType = RotateType::JoystickInput;
	}
	else if (typeString == "ToWall")
	{
		_rotateType = RotateType::ToWall;
	}
	else
	{
		static_assert(static_cast<int>(RotateType::Count) == 6, "타입 추가시 확인");
		ThrowErrCode(ErrCode::UndefinedType, typeString);
	}
}

void FrameEvent_Rotate::process(Actor& actor) const noexcept
{
	check(-MathHelper::Pi <= _offset && _offset < MathHelper::Pi);
	actor.setRotateType(_rotateType, _offset, _rotateSpeed);
}

FrameEvent_Speed::FrameEvent_Speed(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("TargetSpeed", _targetSpeed);
	node.loadAttribute("Acceleration", _acceleration);
	_acceleration *= ACCELERATION_UNIT;

	std::string typeString;
	node.loadAttribute("MoveType", typeString);
	if (typeString == "CharacterDirection")
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
		static_assert(static_cast<int>(MoveType::Count) == 3, "타입 추가시 확인");
		ThrowErrCode(ErrCode::UndefinedType, typeString);
	}
}

void FrameEvent_Speed::process(Actor& actor) const noexcept
{
	check(_acceleration > 0.f);
	check(_targetSpeed >= 0.f);

	actor.setAcceleration(_acceleration, _targetSpeed, _moveType);
}

FrameEvent_Jump::FrameEvent_Jump(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("Speed", _speed);
	node.loadAttribute("TargetFallSpeed", _targetFallSpeed);
	node.loadAttribute("Acceleration", _acceleration);
}

void FrameEvent_Jump::process(Actor& actor) const noexcept
{
	actor.setVerticalSpeed(_speed);
	actor.setTargetVerticalSpeed(_targetFallSpeed, _acceleration);
}

FrameEvent_Die::FrameEvent_Die(const XMLReaderNode& node)
	: FrameEvent(node)
{

}

void FrameEvent_Die::process(Actor& actor) const noexcept
{
	check(SMGFramework::getStageManager() != nullptr);

	SMGFramework::getStageManager()->killActor(&actor);
}

FrameEvent_SpawnCharacter::FrameEvent_SpawnCharacter(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("CharacterKey", _characterKey);
	node.loadAttribute("Position", _position);
	node.loadAttribute("Size", _size);

	if (nullptr == SMGFramework::getCharacterInfoManager()->getInfo(_characterKey))
	{
		ThrowErrCode(ErrCode::InvalidXmlData, std::to_string(_characterKey));
	}
}

void FrameEvent_SpawnCharacter::process(Actor& actor) const noexcept
{
	SpawnInfo spawnInfo;
	spawnInfo._key = _characterKey;
	spawnInfo._position = MathHelper::add(actor.getPosition(), _position);
	spawnInfo._direction = actor.getDirection();
	spawnInfo._upVector = actor.getUpVector();
	spawnInfo._size = _size;

	SMGFramework::getStageManager()->requestSpawn(std::move(spawnInfo));
}

FrameEvent_Effect::FrameEvent_Effect(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("Name", _effectName);
	node.loadAttribute("Position", _positionOffset);
	node.loadAttribute("Size", _size);

	check(SMGFramework::getD3DApp()->hasEffect(_effectName), _effectName);
}

void FrameEvent_Effect::process(Actor& actor) const noexcept
{
	XMVECTOR offsetWorld = actor.getGameObject()->transformLocalToWorld(XMLoadFloat3(&_positionOffset));

	XMFLOAT3 position;
	XMStoreFloat3(&position, offsetWorld);

	EffectInstance instance(position, _size);
	SMGFramework::getD3DApp()->addEffectInstance(_effectName, std::move(instance));
}

FrameEvent_SetVariable::FrameEvent_SetVariable(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("Name", _name);
	node.loadAttribute("Value", _value);
}

void FrameEvent_SetVariable::process(Actor& actor) const noexcept
{
	actor.setActionChartVariable(_name, _value);
}

FrameEvent_FallSpeed::FrameEvent_FallSpeed(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("TargetFallSpeed", _targetFallSpeed);
	node.loadAttribute("Acceleration", _acceleration);
}

void FrameEvent_FallSpeed::process(Actor& actor) const noexcept
{
	actor.setTargetVerticalSpeed(_targetFallSpeed, _acceleration);
}

FrameEvent_SetPath::FrameEvent_SetPath(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("PathKey", _pathKey);
}

void FrameEvent_SetPath::process(Actor& actor) const noexcept
{
	actor.setPath(_pathKey);
}

FrameEvent_Gravity::FrameEvent_Gravity(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("On", _on);
}

void FrameEvent_Gravity::process(Actor& actor) const noexcept
{
	actor.setGravityOn(_on);
}

FrameEvent_SetCamera::FrameEvent_SetCamera(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("CameraKey", _cameraKey);
}

void FrameEvent_SetCamera::process(Actor& actor) const noexcept
{
	check(SMGFramework::getCamera() != nullptr);

	SMGFramework::getCamera()->setInputCameraPointKey(_cameraKey);
}

FrameEvent_Collision::FrameEvent_Collision(const XMLReaderNode& node)
	: FrameEvent(node)
{
	node.loadAttribute("On", _on);
}

void FrameEvent_Collision::process(Actor& actor) const noexcept
{
	actor.setCollisionOn(_on);
}
