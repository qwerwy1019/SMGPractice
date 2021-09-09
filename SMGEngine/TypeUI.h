#pragma once
#include "stdafx.h"

enum class TextFormatType : uint8_t
{
	Normal,
	Points,
	PointsOutline,
	Count,
};
enum class TextBrushType : uint8_t
{
	Black,
	White,

	Count,
};

enum class UIFunctionType
{
	SetHpUI,
	ShowUI,
	HideUI,
	ShakeUI,
	IrisOut,
	IrisIn,
	
	UpdateIris,
	UpdateMousePointer,

	Count,
};

enum class UIElementType
{
	Image,
	Text,
	Iris,
	
	Count,
};

enum class SideType
{
	Left,
	Right,
	Up,
	Down,
};
