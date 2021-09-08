#include "stdafx.h"
#include "UIFunction.h"
#include "UIManager.h"
#include "Exception.h"
#include "StageManager.h"
#include "SMGFramework.h"
#include <algorithm>

std::array<std::function<void(UIGroup*)>, static_cast<int>(UIFunctionType::Count)> UIFunction::_functions =
std::array<std::function<void(UIGroup*)>, static_cast<int>(UIFunctionType::Count)>();

void UIFunction::initialize(void) noexcept
{
	_functions[static_cast<int>(UIFunctionType::SetHpUI)] = setHpUI;

	static_assert(static_cast<int>(UIFunctionType::Count) == 1);
}

void UIFunction::execute(UIFunctionType functionType, UIGroup* uiGroup)
{
	check(uiGroup != nullptr);
	check(functionType != UIFunctionType::Count);

	_functions[static_cast<int>(functionType)](uiGroup);
}

void UIFunction::setHpUI(UIGroup* uiGroup)
{
	check(uiGroup != nullptr);

	UIElement* elementImage = uiGroup->findElement("HPPanelImage");
	if (elementImage == nullptr)
	{
		check(false, "HPPanelImage가 없음.");
		return;
	}
	if (elementImage->getType() != UIElementType::Image)
	{
		check(false, "HPPanelImage Type Error : " + std::to_string(static_cast<int>(elementImage->getType())));
		return;
	}
	UIElementImage* elementImageCasted = static_cast<UIElementImage*>(elementImage);

	UIElement* elementText = uiGroup->findElement("HPText");
	if (elementText == nullptr)
	{
		check(false, "HPText가 없음.");
		return;
	}
	if (elementText->getType() != UIElementType::Text)
	{
		check(false, "HPText Type Error : " + std::to_string(static_cast<int>(elementText->getType())));
		return;
	}
	UIElementText* elementTextCasted = static_cast<UIElementText*>(elementText);
	int hp = SMGFramework::getStageManager()->getStageScriptVariable("PlayerHP");
	hp = std::clamp(hp, 0, 6);
	elementImageCasted->setImage("ui/hpBackground_" + std::to_string(hp));
	elementTextCasted->setText(std::to_wstring(hp));
}
