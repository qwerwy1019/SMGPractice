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
	CheckSpeed,
	Variable,
	CheckVerticalSpeed,
	ActionIndex,
	HasPath,
	PathEnd,
	TargetPositionArrive,
	StageVariable,
	PointerPicked,
	OnWall,
	IsPointerActive,

	Count,
};

enum class FrameEventType
{
	Rotate,
	Speed,
	Jump,
	Die,
	SpawnCharacter,
	Effect,
	SetVariable,
	FallSpeed,
	SetPath,
	Gravity,
	SetCamera,
	Collision,
	TargetPosition,
	AnimationSpeed,
	AddStageVariable,
	EnableEffect,
	DisableEffect,
	SetEffectAlpha,
	CallUIFunction,
	ChangeMaterial,
	StarShoot,

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
	ToPoint,
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
	FixedSpeed,

	Count,
};

enum class CollisionShape
{
	Sphere,
	Box,
	Polygon,
	Line,

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
	SetVariableFromTarget,

	Count,
};