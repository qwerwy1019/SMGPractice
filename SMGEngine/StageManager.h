#pragma once
#include "TypeData.h"

struct StageInfo;

class StageManager
{
	HRESULT loadStage(const std::string& stageName) noexcept;
	StageInfo* _stageInfo;
};