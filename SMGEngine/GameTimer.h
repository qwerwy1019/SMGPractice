#pragma once
#include "TypeCommon.h"

class GameTimer
{
public:
	GameTimer(void) noexcept;
	void ProgressTick(void) noexcept;
	void Reset(void) noexcept;
	void Start(void) noexcept;
	void Stop(void) noexcept;

	TickCount64 getCurrentTickCount(void) const noexcept { return _currentTickCount; }
	TickCount64 getDeltaTickCount(void) const noexcept { return _deltaTickCount; }
	double getDeltaTime(void) const noexcept;
	double getTotalTime(void) const noexcept;
	bool isTimerStopped(void) const noexcept;

private:
	TickCount64 _deltaTickCount;
	TickCount64 _currentTickCount;
	TickCount64 _baseTickCount;
	TickCount64 _pausedTickCount;
	TickCount64 _stopAccumTickCount;

	bool _stopped;
	
	static constexpr int TIME_TO_TICKCOUNT = 1000;
	static constexpr double TICKCOUNT_TO_TIME = 1.f / TIME_TO_TICKCOUNT;
};

