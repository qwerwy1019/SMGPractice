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
	None,
	SetHpUI,
	SetStartBitUI,
	SetCoinUI,
	SetLifeUI,
	ActivateUI,
	InactivateUI,
	ShowUI,
	HideUI,
	ShakeUI,
	IrisOut,
	IrisIn,
	
	UpdateIris,
	UpdateMousePointer,

	Count,
};
static UIFunctionType getUIFunctionTypeFromString(const std::string& typeString) noexcept
{
	if (typeString.empty())
	{
		return UIFunctionType::None;
	}
	else if (typeString == "setHpUI")
	{
		return UIFunctionType::SetHpUI;
	}
	else if (typeString == "setStarBitUI")
	{
		return UIFunctionType::SetStartBitUI;
	}
	else if (typeString == "setCoinUI")
	{
		return UIFunctionType::SetCoinUI;
	}
	else if (typeString == "setLifeUI")
	{
		return UIFunctionType::SetLifeUI;
	}
	else if (typeString == "activateUI")
	{
		return UIFunctionType::ActivateUI;
	}
	else if (typeString == "inactivateUI")
	{
		return UIFunctionType::InactivateUI;
	}
	else if (typeString == "showUI")
	{
		return UIFunctionType::ShowUI;
	}
	else if (typeString == "showUI")
	{
		return UIFunctionType::ShowUI;
	}
	else if (typeString == "hideUI")
	{
		return UIFunctionType::HideUI;
	}
	else if (typeString == "shakeUI")
	{
		return UIFunctionType::ShakeUI;
	}
	else if (typeString == "irisOut")
	{
		return UIFunctionType::IrisOut;
	}
	else if (typeString == "irisIn")
	{
		return UIFunctionType::IrisIn;
	}
	else if (typeString == "updateIris")
	{
		return UIFunctionType::UpdateIris;
	}
	else if (typeString == "updateMousePointer")
	{
		return UIFunctionType::UpdateMousePointer;
	}
	else
	{
		static_assert(static_cast<int>(UIFunctionType::Count) == 14);
		return UIFunctionType::Count;
	}
}

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
