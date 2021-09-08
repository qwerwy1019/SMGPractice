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

private:
	static std::array<std::function<void(UIGroup*)>, static_cast<int>(UIFunctionType::Count)> _functions;
};
