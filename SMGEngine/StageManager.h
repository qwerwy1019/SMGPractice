#pragma once
#include "TypeData.h"

struct StageInfo;

class StageManager
{
	ErrCode loadStage(const std::string& stageName) noexcept;
	StageInfo* _stageInfo;
};