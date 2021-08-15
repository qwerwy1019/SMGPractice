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
	CheckAction,
	CharacterType,

	Count,
};

enum class FrameEventType
{
	Rotate,
	Speed,
	Jump,
	Die,
	SpawnCharacter,

	Count,
};

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
	ToCollidingTarget,

	Count,
};

enum class CollisionShape
{
	Sphere,
	Box,
	Polygon,

	Count,
};

enum class CollisionCase
{
	Upper,
	Center,
	Lower,
	All,
};
enum class CollisionEventType
{
	RotateToTarget,
	SetAction,

	Count,
};