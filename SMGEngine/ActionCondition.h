#pragma once
#include "TypeAction.h"
#include "TypeCommon.h"

class Actor;

class ActionCondition
{
public:
	ActionCondition();
	virtual ~ActionCondition() = default;
	virtual ActionConditionType getType(void) const noexcept = 0;
	virtual bool checkCondition(const Actor& actor) const noexcept = 0;
	bool isNotCondition(void) const noexcept { return _not; }
	void setNotCondition(void) noexcept { _not = true; }
	static std::unique_ptr<ActionCondition> parseConditionString(const std::string& input);
public:
	bool _not;
};
class ActionCondition_Tick : public ActionCondition
{
public:
	ActionCondition_Tick(const std::string& args);
	virtual ~ActionCondition_Tick() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Tick; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	uint32_t _tickStart;
	uint32_t _tickEnd;
};

class ActionCondition_Key : public ActionCondition
{
public:
	ActionCondition_Key(const std::string& args);
	virtual ~ActionCondition_Key() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Key; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	ButtonInputType _buttonType;
	ButtonState _buttonState;
};

class ActionCondition_End : public ActionCondition
{
public:
	ActionCondition_End() = default;
	virtual ~ActionCondition_End() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::End; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
};

class ActionCondition_Stick : public ActionCondition
{
public:
	ActionCondition_Stick(const std::string& args);
	virtual ~ActionCondition_Stick() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Key; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	StickInputType _stickType;
	StickInputState _stickInputState;
};

class ActionCondition_IsStop : public ActionCondition
{
public:
	ActionCondition_IsStop(const std::string& args);
	virtual ~ActionCondition_IsStop() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::IsStop; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_OnGround : public ActionCondition
{
public:
	ActionCondition_OnGround(const std::string& args);
	virtual ~ActionCondition_OnGround() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::OnGround; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_Falling : public ActionCondition
{
public:
	ActionCondition_Falling(const std::string& args);
	virtual ~ActionCondition_Falling() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Falling; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_Random : public ActionCondition
{
public:
	ActionCondition_Random(const std::string& args);
	virtual ~ActionCondition_Random() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Random; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	uint8_t _probability;
};

class ActionCondition_CheckPlayerDistance : public ActionCondition
{
public:
	ActionCondition_CheckPlayerDistance(const std::string& args);
	virtual ~ActionCondition_CheckPlayerDistance() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::CheckPlayerDistance; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	int _min;
	int _max;
};

class ActionCondition_CheckPlayerAltitude : public ActionCondition
{
public:
	ActionCondition_CheckPlayerAltitude(const std::string& args);
	virtual ~ActionCondition_CheckPlayerAltitude() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::CheckPlayerAltitude; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	int _min;
	int _max;
};

class ActionCondition_CheckAction : public ActionCondition
{
public:
	ActionCondition_CheckAction(const std::string& args);
	virtual ~ActionCondition_CheckAction() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::CheckAction; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	std::string _actionStateName;
};

class ActionCondition_CharacterType : public ActionCondition
{
public:
	ActionCondition_CharacterType(const std::string& args);
	virtual ~ActionCondition_CharacterType() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::CharacterType; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	CharacterType _characterType;
};

class ActionCondition_CheckSpeed : public ActionCondition
{
public:
	ActionCondition_CheckSpeed(const std::string& args);
	virtual ~ActionCondition_CheckSpeed() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::CheckSpeed; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	float _min;
	float _max;
};

class ActionCondition_Variable : public ActionCondition
{
public:
	ActionCondition_Variable(const std::string& args);
	virtual ~ActionCondition_Variable() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::Variable; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	std::string _name;
	int _value;
};

class ActionCondition_CheckVerticalSpeed : public ActionCondition
{
public:
	ActionCondition_CheckVerticalSpeed(const std::string& args);
	virtual ~ActionCondition_CheckVerticalSpeed() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::CheckVerticalSpeed; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	float _min;
	float _max;
};

class ActionCondition_ActionIndex : public ActionCondition
{
public:
	ActionCondition_ActionIndex(const std::string& args);
	virtual ~ActionCondition_ActionIndex() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::ActionIndex; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	int _actionIndex;
};

class ActionCondition_HasPath : public ActionCondition
{
public:
	ActionCondition_HasPath(const std::string& args);
	virtual ~ActionCondition_HasPath() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::HasPath; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_PathEnd : public ActionCondition
{
public:
	ActionCondition_PathEnd(const std::string& args);
	virtual ~ActionCondition_PathEnd() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::PathEnd; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_TargetPositionArrive : public ActionCondition
{
public:
	ActionCondition_TargetPositionArrive(const std::string& args);
	virtual ~ActionCondition_TargetPositionArrive() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::TargetPositionArrive; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_StageVariable : public ActionCondition
{
public:
	ActionCondition_StageVariable(const std::string& args);
	virtual ~ActionCondition_StageVariable() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::StageVariable; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
	std::string _variableName;
	OperatorType _operator;
	int _value;
};

class ActionCondition_PointerPicked : public ActionCondition
{
public:
	ActionCondition_PointerPicked(const std::string& args);
	virtual ~ActionCondition_PointerPicked() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::PointerPicked; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_OnWall : public ActionCondition
{
public:
	ActionCondition_OnWall(const std::string& args);
	virtual ~ActionCondition_OnWall() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::OnWall; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};

class ActionCondition_IsPointerActive : public ActionCondition
{
public:
	ActionCondition_IsPointerActive(const std::string& args);
	virtual ~ActionCondition_IsPointerActive() = default;
	virtual ActionConditionType getType(void) const noexcept override { return ActionConditionType::IsPointerActive; }
	virtual bool checkCondition(const Actor& actor) const noexcept override;
private:
};