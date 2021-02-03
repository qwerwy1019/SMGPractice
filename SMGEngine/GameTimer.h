#pragma once
class GameTimer
{
public:
	GameTimer(void) noexcept;
	void ProgressTick(void) noexcept;
	void Reset(void) noexcept;
	void Start(void) noexcept;
	void Stop(void) noexcept;

	double getDeltaTime(void) const noexcept;
	double getTotalTime(void) const noexcept;
	bool isTimerStopped(void) const noexcept;

private:
	double _secondPerCount;
	double _deltaTime;

	__int64 _currentCount;
	__int64 _baseCount;
	__int64 _pausedCount;
	__int64 _stopAccumCount;

	bool _stopped;
};

