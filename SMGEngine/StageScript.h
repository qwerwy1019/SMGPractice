#pragma once

class XMLReaderNode;
class StagePhaseFunction;
class StagePhaseCondition;

class StagePhaseBranch
{
public:
	StagePhaseBranch(const XMLReaderNode& node);
	const std::string& getStagePhase() const noexcept { return _stagePhase; }
	bool checkBranchCondition(void) const noexcept;
private:
	std::string _stagePhase;
	std::vector<std::unique_ptr<StagePhaseCondition>> _conditions;
};
class StagePhase
{
public:
	StagePhase(const XMLReaderNode& node);
	~StagePhase();
	bool getNextPhaseName(std::string& nextPhaseName) const noexcept;
	void processFunctions(void) noexcept;
private:
	std::vector<StagePhaseBranch> _branches;
	std::vector<std::unique_ptr<StagePhaseFunction>> _functions;
};

class StageScript
{
public:
	StageScript(const XMLReaderNode& node);
	void loadXMLVariables(const XMLReaderNode& node);
	void loadXMLPhases(const XMLReaderNode& node);
	const std::unordered_map<std::string, int>& getVariables(void) const noexcept;
	StagePhase* getStartPhase(void) const noexcept;
	StagePhase* getPhase(const std::string& name) const noexcept;
private:
	std::unordered_map<std::string, std::unique_ptr<StagePhase>> _phases;
	std::unordered_map<std::string, int> _variables;
	std::string _startPhaseName;
};