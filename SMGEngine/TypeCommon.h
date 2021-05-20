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

	LMouseButton,
	RMouseButton,
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

using CharacterKey = uint16_t;