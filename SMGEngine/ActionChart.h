#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"

class Actor;
class XMLReaderNode;
class ActionCondition;
class FrameEvent;
class CollisionHandler;

class ActionChart;

class ActionBranch
{
public:
	ActionBranch(const XMLReaderNode& node);
	std::string getActionState() const noexcept { return _actionState; }
	bool checkBranchCondition(const Actor& actor) const noexcept;
private:
	std::string _actionState;
	std::vector<std::unique_ptr<ActionCondition>> _actionConditions;
};

class ActionState
{
public:
	ActionState(const XMLReaderNode& node);
	bool checkBranch(Actor& actor, std::string& nextState) const noexcept;
	void processFrameEvents(Actor& actor, const TickCount64& lastProcessedTick, const TickCount64& progressedTick) const noexcept;
	std::string getAnimationName(void) const noexcept;
	TickCount64 getBlendTick(void) const noexcept;
	void checkValid(const ActionChart* actionChart) const;
private:
	std::vector<std::unique_ptr<FrameEvent>> _frameEvents;
	std::vector<ActionBranch> _branches;

	std::string _animationName;
	TickCount64 _blendTick;
};

struct ChildEffectInfo
{
	std::string _effectName;
	DirectX::XMFLOAT3 _positionOffset;
	float _size;
};

class ActionChart
{
public:
	ActionChart(const XMLReaderNode& node);
	~ActionChart();
	
	ActionState* getActionState(const std::string& name) const noexcept;
	void checkValid(void) const;
	void processCollisionHandlers(Actor& selfActor, const Actor& targetActor, CollisionCase collisionCase) const noexcept;
	const std::unordered_map<std::string, int>& getVariables(void) const noexcept;
	bool getChildEffectInfo(int key, ChildEffectInfo& outInfo) const noexcept;
private:
	void loadXMLActionStates(const XMLReaderNode& node);
	void loadXMLCollisionHandlers(const XMLReaderNode& node);
	void loadXMLVariables(const XMLReaderNode& node);
	void loadXMLChildEffects(const XMLReaderNode& node);
private:
	std::unordered_map<std::string, std::unique_ptr<ActionState>> _actionStates;
	std::vector<std::unique_ptr<CollisionHandler>> _collisionHandlers;
	std::unordered_map<std::string, int> _variables;
	std::unordered_map<int, ChildEffectInfo> _childEffects;
};