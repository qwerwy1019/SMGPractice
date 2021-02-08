#pragma once
#include "TypeD3d.h"

struct StageInfo;
enum class ErrCode : uint32_t;

class StageManager
{
	ErrCode loadStage(const std::string& stageName) noexcept;
	StageInfo* _stageInfo;
};