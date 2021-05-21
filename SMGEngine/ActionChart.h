#pragma once
#include "TypeCommon.h"

class Actor;
class XMLReaderNode;
class ActionCondition;
class FrameEvent;

class ActionBranch
{
public:
	void loadXML(const XMLReaderNode& node);
	std::string getActionState() const noexcept { return _actionState; }
	bool checkBranchCondition(const Actor& actor) const noexcept;
private:
	std::string _actionState;
	std::vector<std::unique_ptr<ActionCondition>> _actionConditions;
};

class ActionState
{
public:
	void loadXML(const XMLReaderNode& node);
	bool checkBranch(Actor& actor, std::string& nextState) const noexcept;
	void processFrameEvents(Actor& actor, const TickCount64& lastProcessedTick, const TickCount64& progressedTick) const noexcept;
	std::string getAnimationName(void) const noexcept;
	TickCount64 getBlendTick(void) const noexcept;
private:
	std::vector<std::unique_ptr<FrameEvent>> _frameEvents;
	std::vector<ActionBranch> _branches;

	std::string _animationName;
	TickCount64 _blendTick;
};
class ActionChart
{
public:
	ActionChart(const XMLReaderNode& node);
	//void updateActionState(Actor& actor, uint32_t deltaFrame) const noexcept;
	ActionState* getActionState(const std::string& name) const noexcept;
private:
	std::unordered_map<std::string, std::unique_ptr<ActionState>> _actionStates;

};