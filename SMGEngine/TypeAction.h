#pragma once
#include "TypeCommon.h"

enum class ActionConditionType
{
	Tick,
	Key,
	End,
	IsStop,
	OnGround,
	Falling,
	Random,
	CheckPlayerDistance,
	CheckPlayerAltitude,

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
	CharacterDirection,
	JoystickDirection,
	Path,
	Count,
};

enum class RotateType
{
	Fixed,
	ToPlayer,
	Path,
	JoystickInput,
	ToWall,

	Count,
};

enum class CollisionShape
{
	Sphere,
	Box,
	Polygon,

	Count,
};
