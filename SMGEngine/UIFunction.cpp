#include "stdafx.h"
#include "UIFunction.h"
#include "UIManager.h"
#include "Exception.h"
#include "StageManager.h"
#include "SMGFramework.h"
#include <algorithm>
#include "MathHelper.h"

std::array<std::function<void(UIGroup*)>, static_cast<int>(UIFunctionType::Count)> UIFunction::_functions =
std::array<std::function<void(UIGroup*)>, static_cast<int>(UIFunctionType::Count)>();

void UIFunction::initialize(void) noexcept
{
	_functions[static_cast<int>(UIFunctionType::SetHpUI)] = setHpUI;
	_functions[static_cast<int>(UIFunctionType::ShowUI)] = showUI;
	_functions[static_cast<int>(UIFunctionType::HideUI)] = hideUI;
	_functions[static_cast<int>(UIFunctionType::ShakeUI)] = shakeUI;
	_functions[static_cast<int>(UIFunctionType::IrisOut)] = irisOut;
	_functions[static_cast<int>(UIFunctionType::IrisIn)] = irisIn;
	_functions[static_cast<int>(UIFunctionType::UpdateIris)] = updateIris;
	_functions[static_cast<int>(UIFunctionType::UpdateMousePointer)] = updateMousePointer;

	static_assert(static_cast<int>(UIFunctionType::Count) == 8);
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

void UIFunction::showUI(UIGroup* uiGroup)
{
	check(uiGroup != nullptr);
	uiGroup->setMove(DirectX::XMFLOAT2(0, 0), 300, InterpolationType::Cosine);
}

void UIFunction::hideUI(UIGroup* uiGroup)
{
	check(uiGroup != nullptr);
	uiGroup->setMove(uiGroup->getHidePositionOffset(), 300, InterpolationType::Cosine);
}

void UIFunction::shakeUI(UIGroup* uiGroup)
{
	check(uiGroup != nullptr);
	DirectX::XMFLOAT2 toPosition = uiGroup->getCurrentPositionOffset();
	toPosition = MathHelper::add(toPosition, DirectX::XMFLOAT2(0, 5));
	uiGroup->setMove(toPosition, 0, InterpolationType::Shake);
}

void UIFunction::irisOut(UIGroup* uiGroup)
{
	check(uiGroup != nullptr);
	UIElement* uiElement = uiGroup->findElement("Iris");
	if (uiElement == nullptr)
	{
		check(false, "Iris가 없음.");
		return;
	}
	if (uiElement->getType() != UIElementType::Iris)
	{
		check(false, "Iris element type error " + std::to_string(static_cast<int>(uiElement->getType())));
		return;
	}

	UIElementIris* iris = static_cast<UIElementIris*>(uiElement);
	iris->set(false);
}

void UIFunction::irisIn(UIGroup* uiGroup)
{
	check(uiGroup != nullptr);
	UIElement* uiElement = uiGroup->findElement("Iris");
	if (uiElement == nullptr)
	{
		check(false, "Iris가 없음.");
		return;
	}
	if (uiElement->getType() != UIElementType::Iris)
	{
		check(false, "Iris element type error " + std::to_string(static_cast<int>(uiElement->getType())));
		return;
	}

	UIElementIris* iris = static_cast<UIElementIris*>(uiElement);
	iris->set(true);
}

void UIFunction::updateIris(UIGroup* uiGroup)
{
	check(uiGroup != nullptr);
	UIElement* uiElement = uiGroup->findElement("Iris");
	if (uiElement == nullptr)
	{
		check(false, "Iris가 없음.");
		return;
	}
	if (uiElement->getType() != UIElementType::Iris)
	{
		check(false, "Iris element type error " + std::to_string(static_cast<int>(uiElement->getType())));
		return;
	}

	UIElementIris* iris = static_cast<UIElementIris*>(uiElement);
	iris->update();
}

void UIFunction::updateMousePointer(UIGroup* group)
{
	check(group != nullptr);
	if (!SMGFramework::Get().isPointerActive())
	{
		group->setActive(false);
		return;
	}

	group->setActive(true);
	const auto& mousePoint = SMGFramework::Get().getMousePos();
	group->setMove(mousePoint, 0, InterpolationType::Linear);
}
