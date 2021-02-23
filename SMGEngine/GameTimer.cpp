#include "stdafx.h"
#include "GameTimer.h"
#include "Exception.h"

GameTimer::GameTimer() noexcept
	: _deltaTickCount(0)
	, _currentTickCount(0)
	, _baseTickCount(0)
	, _pausedTickCount(0)
	, _stopAccumTickCount(0)
	, _stopped(false)
{
}

void GameTimer::ProgressTick() noexcept
{
	if (_stopped)
	{
		_deltaTickCount = 0;
		return;
	}

	TickCount64 currentCount = GetTickCount64();
	check(_currentTickCount <= currentCount, "시간이 역행합니다.");
	_deltaTickCount = (currentCount - _currentTickCount);
	_currentTickCount = currentCount;
}

double GameTimer::getDeltaTime(void) const noexcept
{
	return _deltaTickCount * TICKCOUNT_TO_TIME;
}

double GameTimer::getTotalTime(void) const noexcept
{
	return (_currentTickCount - _baseTickCount - _stopAccumTickCount) * TICKCOUNT_TO_TIME;
}

void GameTimer::Reset(void) noexcept
{
	_deltaTickCount = 0;
	_currentTickCount = GetTickCount64();
	_baseTickCount = _currentTickCount;
	_pausedTickCount = 0;
	_stopAccumTickCount = 0;
	_stopped = false;
}

void GameTimer::Start(void) noexcept
{
	if (!_stopped)
	{
		return;
	}
	TickCount64 currentCount = GetTickCount64();
	_stopAccumTickCount += currentCount - _pausedTickCount;
	_currentTickCount += currentCount - _pausedTickCount;

	_stopped = false;
}

void GameTimer::Stop(void) noexcept
{
	if (_stopped)
	{
		return;
	}
	_pausedTickCount = GetTickCount64();

	_stopped = true;
}

bool GameTimer::isTimerStopped(void) const noexcept
{
	return _stopped;
}
