#pragma once
#include "TypeCommon.h"
class XMLReaderNode;

enum class CameraPointType
{
	Fixed,
	PlayerFocus,
	PlayerFocusFixed,
	Path,

	Count,
};

enum class LandscapeType
{
	Basic,
	Galaxy,

	Count,
};

enum class GravityPointType
{
	Fixed,
	Point,
	GroundNormal,

	Count,
};
