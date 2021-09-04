#pragma once
#include "TypeStage.h"

class StagePhaseCondition;
class XMLReaderNode;

class StagePhaseFunction
{
public:
	StagePhaseFunction(const XMLReaderNode& node);
	virtual ~StagePhaseFunction() = default;
	virtual void process(void) noexcept;
	virtual StagePhaseFunctionType getType() const noexcept = 0;
	bool checkConditions(void) const noexcept;
	static std::unique_ptr<StagePhaseFunction> loadXMLFunction(const XMLReaderNode& node);
private:
	std::vector<std::unique_ptr<StagePhaseCondition>> _conditions;
	int _processCount;
	int _processCountInit;
};

class StagePhaseFunction_SpawnActor : public StagePhaseFunction
{
public:
	StagePhaseFunction_SpawnActor(const XMLReaderNode& node);
	virtual ~StagePhaseFunction_SpawnActor() = default;
	virtual void process(void) noexcept;
	virtual StagePhaseFunctionType getType() const noexcept override { return StagePhaseFunctionType::SpawnActor; }
private:
	int _spawnKey;
};

class StagePhaseFunction_RemoveLife : public StagePhaseFunction
{
public:
	StagePhaseFunction_RemoveLife(const XMLReaderNode& node);
	virtual ~StagePhaseFunction_RemoveLife() = default;
	virtual void process(void) noexcept;
	virtual StagePhaseFunctionType getType() const noexcept override { return StagePhaseFunctionType::RemoveLife; }
private:
};

class StagePhaseFunction_LoadStage : public StagePhaseFunction
{
public:
	StagePhaseFunction_LoadStage(const XMLReaderNode& node);
	virtual ~StagePhaseFunction_LoadStage() = default;
	virtual void process(void) noexcept;
	virtual StagePhaseFunctionType getType() const noexcept override { return StagePhaseFunctionType::LoadStage; }
private:
	std::string _stageName;
};