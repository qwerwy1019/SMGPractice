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

	Count,
};

enum class UIElementType
{
	Image,
	Text,
	
	Count,
};