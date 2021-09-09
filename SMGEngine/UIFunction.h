#pragma once
#include "TypeCommon.h"
#include "TypeUI.h"
#include <functional>

class UIGroup;

class UIFunction
{
public:
	static void initialize(void) noexcept;
	static void execute(UIFunctionType functionType, UIGroup* uiGroup);

	static void setHpUI(UIGroup* uiGroup);
	static void showUI(UIGroup* uiGroup);
	static void hideUI(UIGroup* uiGroup);
	static void shakeUI(UIGroup* uiGroup);
	static void irisOut(UIGroup* uiGroup);
	static void irisIn(UIGroup* uiGroup);

	static void updateIris(UIGroup* uiGroup);
	static void updateMousePointer(UIGroup* group);

private:
	static std::array<std::function<void(UIGroup*)>, static_cast<int>(UIFunctionType::Count)> _functions;
};
