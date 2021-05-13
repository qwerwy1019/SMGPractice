#include "stdafx.h"
#include "ActionCondition.h"
#include "Actor.h"
#include "Exception.h"
#include "SMGFramework.h"


void ActionCondition_Tick::loadXML(const std::string& condition)
{

}

bool ActionCondition_Tick::checkCondition(const Actor& actor) const noexcept
{
	uint32_t current = actor.getLocalTickCount();
	if (_frameStart <= current && current < _frameEnd)
	{
		return true;
	}
	return false;
}

bool ActionCondition_End::checkCondition(const Actor& actor) const noexcept
{
	if (actor.isActionEnd())
	{
		return true;
	}
	return false;
}

void ActionCondition_Button::loadXML(const std::string& condition)
{

}

bool ActionCondition_Button::checkCondition(const Actor& actor) const noexcept
{
	if (_buttonState == SMGFramework::Get().getButtonInput(_buttonType))
	{
		return true;
	}

	return false;
}
