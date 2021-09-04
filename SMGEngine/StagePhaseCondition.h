#pragma once
#include "TypeStage.h"

class XMLReaderNode;

class StagePhaseCondition
{
public:
	StagePhaseCondition();
	virtual ~StagePhaseCondition() = default;
	virtual StagePhaseConditionType getType(void) const noexcept = 0;
	virtual bool checkCondition(void) const noexcept = 0;
	bool isNotCondition(void) const noexcept { return _not; }
	void setNotCondition(void) noexcept { _not = true; }
	static std::unique_ptr<StagePhaseCondition> parseConditionString(const std::string& input);
private:
	bool _not;
};

class StagePhaseCondition_Variable : public StagePhaseCondition
{
public:
	StagePhaseCondition_Variable(const std::string& args);
	virtual ~StagePhaseCondition_Variable() = default;
	virtual StagePhaseConditionType getType(void) const noexcept override { return StagePhaseConditionType::Variable; }
	virtual bool checkCondition(void) const noexcept override;
private:
	std::string _name;
	OperatorType _operator;
	int _value;
};

class StagePhaseCondition_UserData : public StagePhaseCondition
{
public:
	StagePhaseCondition_UserData(const std::string& args);
	virtual ~StagePhaseCondition_UserData() = default;
	virtual StagePhaseConditionType getType(void) const noexcept override { return StagePhaseConditionType::Variable; }
	virtual bool checkCondition(void) const noexcept override;
private:
	std::string _name;
	int _value;
};