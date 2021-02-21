#include "stdafx.h"
#include "GameTimer.h"
#include "Exception.h"

GameTimer::GameTimer() noexcept
	: _deltaTickCount(0)
	, _currentCount(0)
	, _baseCount(0)
	, _pausedCount(0)
	, _stopAccumCount(0)
	, _stopped(false)
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&_countPerSec);
	_secondPerCount = 1.f / (double)_countPerSec;
	_countPerTickCount = _countPerSec / 1000;
}

void GameTimer::ProgressTick() noexcept
{
	if (_stopped)
	{
		_deltaTickCount = 0;
		return;
	}

	__int64 currentCount = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentCount);
	check(_currentCount <= currentCount, "시간이 역행합니다.");
	_deltaTickCount = (currentCount - _currentCount) / _countPerTickCount;
	_currentCount = currentCount;
}

double GameTimer::getDeltaTime(void) const noexcept
{
	return _deltaTickCount * _secondPerCount;
}

double GameTimer::getTotalTime(void) const noexcept
{
	return (_currentCount - _baseCount - _stopAccumCount) * _secondPerCount;
}

void GameTimer::Reset(void) noexcept
{
	_deltaTickCount = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&_currentCount);
	_baseCount = _currentCount;
	_pausedCount = 0;
	_stopAccumCount = 0;
	_stopped = false;
}

void GameTimer::Start(void) noexcept
{
	if (!_stopped)
	{
		return;
	}
	__int64 currentCount = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentCount);
	_stopAccumCount += currentCount - _pausedCount;
	_currentCount += currentCount - _pausedCount;

	_stopped = false;
}

void GameTimer::Stop(void) noexcept
{
	if (_stopped)
	{
		return;
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&_pausedCount);

	_stopped = true;
}

bool GameTimer::isTimerStopped(void) const noexcept
{
	return _stopped;
}
