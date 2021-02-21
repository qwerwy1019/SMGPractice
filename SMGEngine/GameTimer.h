#pragma once

class GameTimer
{
public:
	GameTimer(void) noexcept;
	void ProgressTick(void) noexcept;
	void Reset(void) noexcept;
	void Start(void) noexcept;
	void Stop(void) noexcept;

	__int64 getDeltaTickCount(void) const noexcept { return _deltaTickCount; }
	double getDeltaTime(void) const noexcept;
	double getTotalTime(void) const noexcept;
	bool isTimerStopped(void) const noexcept;

private:
	double _secondPerCount;
	__int64 _countPerSec;
	__int64 _countPerTickCount;

	__int64 _deltaTickCount;
	__int64 _currentCount;
	__int64 _baseCount;
	__int64 _pausedCount;
	__int64 _stopAccumCount;

	bool _stopped;
};

