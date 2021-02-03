#include "stdafx.h"
#include "GameTimer.h"

GameTimer::GameTimer() noexcept
	: _deltaTime(0)
	, _currentCount(0)
	, _baseCount(0)
	, _pausedCount(0)
	, _stopAccumCount(0)
	, _stopped(false)
{
	__int64 countPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countPerSec);
	_secondPerCount = 1.f / (double)countPerSec;
}

void GameTimer::ProgressTick() noexcept
{
	if (_stopped)
	{
		_deltaTime = 0.f;
		return;
	}
	
	__int64 currentCount = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentCount);
	_deltaTime = (currentCount - _currentCount) * _secondPerCount;
	_currentCount = currentCount;

	_deltaTime = (_deltaTime < 0.f) ? 0.f : _deltaTime;
}

double GameTimer::getDeltaTime(void) const noexcept
{
	return _deltaTime;
}

double GameTimer::getTotalTime(void) const noexcept
{
	return (_currentCount - _baseCount - _stopAccumCount) * _secondPerCount;
}

void GameTimer::Reset(void) noexcept
{
	_deltaTime = 0.f;
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
