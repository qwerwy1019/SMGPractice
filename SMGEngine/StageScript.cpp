#include "stdafx.h"
#include "StageScript.h"
#include "FileHelper.h"
#include "StagePhaseFunction.h"
#include "StagePhaseCondition.h"

StagePhase::StagePhase(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		const std::string& nodeName = childNode.getNodeName();
		if (nodeName == "Branch")
		{
			_branches.emplace_back(childNode);
		}
		else if (nodeName == "Function")
		{
			_functions.emplace_back(StagePhaseFunction::loadXMLFunction(childNode));
		}
		else
		{
			ThrowErrCode(ErrCode::InvalidXmlData, nodeName);
		}
	}
}

StagePhase::~StagePhase()
{

}

bool StagePhase::getNextPhaseName(std::string& nextPhaseName) const noexcept
{
	for (const auto& branch : _branches)
	{
		if (branch.checkBranchCondition())
		{
			nextPhaseName = branch.getStagePhase();
			return true;
		}
	}
	return false;
}

void StagePhase::processFunctions(void) noexcept
{
	for (const auto& function : _functions)
	{
		if (function->checkConditions())
		{
			function->process();
		}
	}
}

StageScript::StageScript(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodesWithName();

	auto childNode = childNodes.find("Variables");
	loadXMLVariables(childNode->second);

	childNode = childNodes.find("Phases");
	loadXMLPhases(childNode->second);
	
}

void StageScript::loadXMLVariables(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		std::string name;
		int value;
		childNode.loadAttribute("Name", name);
		childNode.loadAttribute("Value", value);
		auto it = _variables.emplace(name, value);
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, name);
		}
	}
}

void StageScript::loadXMLPhases(const XMLReaderNode& node)
{
	node.loadAttribute("Start", _startPhaseName);

	const auto& childNodes = node.getChildNodes();
	for (const auto& childNode : childNodes)
	{
		std::string name = childNode.getNodeName();
		auto it = _phases.emplace(name, std::make_unique<StagePhase>(childNode));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, name);
		}
	}

	if (_phases.find(_startPhaseName) == _phases.end())
	{
		ThrowErrCode(ErrCode::InvalidXmlData, _startPhaseName + " phase 목록에 없음.");
	}
}

const std::unordered_map<std::string, int>& StageScript::getVariables(void) const noexcept
{
	return _variables;
}

StagePhase* StageScript::getStartPhase(void) const noexcept
{
	auto it = _phases.find(_startPhaseName);
	check(it != _phases.end());
	return it->second.get();
}

StagePhase* StageScript::getPhase(const std::string& name) const noexcept
{
	auto it = _phases.find(name);
	check(it != _phases.end());
	return it->second.get();
}

StagePhaseBranch::StagePhaseBranch(const XMLReaderNode& node)
{
	node.loadAttribute("Phase", _stagePhase);
	std::string conditionStrings;
	node.loadAttribute("Condition", conditionStrings);

	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	_conditions.reserve(conditionTokenized.size());
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<StagePhaseCondition> condition = StagePhaseCondition::parseConditionString(c);
		check(condition != nullptr);
		_conditions.push_back(std::move(condition));
	}
}

bool StagePhaseBranch::checkBranchCondition(void) const noexcept
{
	for (const auto& condition : _conditions)
	{
		if (condition->isNotCondition() == condition->checkCondition())
		{
			return false;
		}
	}
	return true;
}
