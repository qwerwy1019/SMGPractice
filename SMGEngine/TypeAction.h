#pragma once
#include "TypeCommon.h"

enum class ActionConditionType
{
	Tick,
	Key,
	End,

	Count,
};

enum class FrameEventType
{
	Rotate,
	Speed,
	Jump,

	Count,
};
#define convertStringToFrameEventType(string) convertStringToType<ActionConditionType>(string, actionConditionString);

enum class CollisionType
{
	SolidObject,
	Character,
	Item,

	Count,
};

enum class MoveType
{
	Fixed,
	CharacterDirection,
	JoystickDirection,
	Path,
	Count,
};

enum class RotateType
{
	ToTarget,
	Path,
	Input,
	Count,
};

enum class CollisionShape
{
	Sphere,
	Box,
	Polygon,

	Count,
};
