#include "stdafx.h"
#include "StagePhaseFunction.h"
#include "FileHelper.h"
#include "StagePhaseCondition.h"
#include "SMGFramework.h"
#include "StageManager.h"
#include "StageInfo.h"
#include "UserData.h"

StagePhaseFunction::StagePhaseFunction(const XMLReaderNode& node)
{
	std::string conditionStrings;
	node.loadAttribute("Condition", conditionStrings);

	std::vector<std::string> conditionTokenized = D3DUtil::tokenizeString(conditionStrings, ' ');
	for (const auto& c : conditionTokenized)
	{
		std::unique_ptr<StagePhaseCondition> condition = StagePhaseCondition::parseConditionString(c);
		check(condition != nullptr);
		_conditions.push_back(std::move(condition));
	}

	node.loadAttribute("ProcessCount", _processCountInit);
	_processCount = _processCountInit;
}

void StagePhaseFunction::process(void) noexcept
{
	_processCount -= 1;
}

bool StagePhaseFunction::checkConditions(void) const noexcept
{
	if (_processCount == 0)
	{
		return false;
	}

	for (const auto& condition : _conditions)
	{
		if (condition->isNotCondition() == condition->checkCondition())
		{
			return false;
		}
	}
	return true;
}

std::unique_ptr<StagePhaseFunction> StagePhaseFunction::loadXMLFunction(const XMLReaderNode& node)
{
	std::string nameString;
	node.loadAttribute("Name", nameString);
	if (nameString == "SpawnActor")
	{
		return std::make_unique<StagePhaseFunction_SpawnActor>(node);
	}
	else if (nameString == "RemoveLife")
	{
		return std::make_unique<StagePhaseFunction_RemoveLife>(node);
	}
	else if (nameString == "LoadStage")
	{
		return std::make_unique<StagePhaseFunction_LoadStage>(node);
	}
	else
	{
		static_assert(static_cast<int>(StagePhaseFunctionType::Count) == 3, "타입추가시 확인할것");
		ThrowErrCode(ErrCode::UndefinedType, nameString);
	}
}

StagePhaseFunction_SpawnActor::StagePhaseFunction_SpawnActor(const XMLReaderNode& node)
	: StagePhaseFunction(node)
{
	check(SMGFramework::getStageManager() != nullptr);
	check(SMGFramework::getStageManager()->getStageInfo() != nullptr);
	node.loadAttribute("Key", _spawnKey);

	if (SMGFramework::getStageManager()->getStageInfo()->checkSpawnInfoWithKey(_spawnKey) == false)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, std::to_string(_spawnKey));
	}
}

void StagePhaseFunction_SpawnActor::process(void) noexcept
{
	StagePhaseFunction::process();

	check(SMGFramework::getStageManager() != nullptr);
	SMGFramework::getStageManager()->requestSpawnWithKey(_spawnKey);
}

StagePhaseFunction_LoadStage::StagePhaseFunction_LoadStage(const XMLReaderNode& node)
	: StagePhaseFunction(node)
{
	node.loadAttribute("StageName", _stageName);
}

void StagePhaseFunction_LoadStage::process(void) noexcept
{
	StagePhaseFunction::process();

	SMGFramework::getStageManager()->setNextStage(_stageName);
}

StagePhaseFunction_RemoveLife::StagePhaseFunction_RemoveLife(const XMLReaderNode& node)
	: StagePhaseFunction(node)
{

}

void StagePhaseFunction_RemoveLife::process(void) noexcept
{
	StagePhaseFunction::process();

	SMGFramework::getUserData()->decreaseLife();
}
