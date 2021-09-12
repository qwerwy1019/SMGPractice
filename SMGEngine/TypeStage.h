#pragma once
#include "TypeCommon.h"
class XMLReaderNode;

enum class CameraPointType
{
	Fixed,
	PlayerFocus,
	PlayerFocusFixed,
	Path,
	CenterFocus,

	Count,
};

enum class GravityPointType
{
	Fixed,
	Point,
	GroundNormal,

	Count,
};

enum class StagePhaseFunctionType
{
	SpawnActor,
	RemoveLife,
	LoadStage,
	//LoadLastSaveFile,
	AddLife,

	Count,
};

enum class StagePhaseConditionType
{
	Variable,
	UserData,

	Count,
};