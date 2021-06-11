#pragma once
#include "stdafx.h"

using TickCount64 = uint64_t;

static constexpr int FRAME_TO_TICKCOUNT = 20;
static constexpr double TICKCOUNT_TO_FRAME = 1.f / FRAME_TO_TICKCOUNT;

enum class ButtonInputType
{
	AB,
	XY,
	ZL,
	ZR,
	L,
	R,

	LStickButton,
	RStickButton,
	Count,
};

enum class ButtonState
{
	Down,
	Press,
	Up,

	None,
};

enum class StickInputType
{
	LStick,
	RStick,
	Pointer,

	Count,
};

enum class StickInputState
{
	None			= 0,
	FrontLong		= 1 << 0,
	LeftLong		= 1 << 1,
	BackLong		= 1 << 2,
	RightLong		= 1 << 3,

	FrontShort		= 1 << 4,
	LeftShort		= 1 << 5,
	BackShort		= 1 << 6,
	RightShort		= 1 << 7,

	Front			= FrontLong | FrontShort,
	Left			= LeftLong | LeftShort,
	Back			= BackLong | BackShort,
	Right			= RightLong | RightShort,


	Long			= FrontLong | LeftLong | BackLong | RightLong,
	Short			= FrontShort | LeftShort | BackShort | RightShort,
	
	Move			= Long | Short,
};
DEFINE_ENUM_FLAG_OPERATORS(StickInputState);

using CharacterKey = uint16_t;
enum class CharacterType
{
	Player,
	Monster,
	Object,

	Count,
};