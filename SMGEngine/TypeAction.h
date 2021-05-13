#pragma once

enum class ActionConditionType
{
	Tick,
	Button,
	End,

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
