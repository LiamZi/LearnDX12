#include "Timer.h"

#include <Windows.h>
#include <new>

 
namespace Nix {

    Timer::Timer()
    :_secondsPerCount(0.0)
    ,_deltaTime(0.0)
    ,_baseTime(0)
    ,_pausedTime(0)
    ,_stopTime(0)
    ,_prevTime(0)
    ,_currTime(0)
    ,_stopped(false)
    {
        int64_t countsPreSec = 0;
        QueryPerformanceFrequency((LARGE_INTEGER *)&countsPreSec);
        _secondsPerCount = 1.0 / (double)countsPreSec;
    }

    Timer::~Timer()
    {

    }

    float Timer::totalTime() const 
    {
        if(_stopped) 
            return (float)(((_stopTime - _pausedTime) - _baseTime) * _secondsPerCount);
        else
            return (float)(((_currTime - _pausedTime) - _baseTime) * _secondsPerCount);
    }

    float Timer::deltaTime() const
    {
        return (float)_deltaTime;
    }

    void Timer::reset()
    {
        int64_t currTime = 0;
        QueryPerformanceCounter((LARGE_INTEGER *) &currTime);

        _baseTime = currTime;
        _prevTime = currTime;
        _stopTime = 0;
        _stopped = false;
    }

    void Timer::start()
    {
        int64_t startTime = 0;
        QueryPerformanceCounter((LARGE_INTEGER *) &startTime);

        if(!_stopped) return;

        _pausedTime += startTime - _stopTime;
        _prevTime = startTime;
        _stopTime = 0;
        _stopped = false;
    }

    void Timer::stop()
    {
        if(_stopped) return;

        int64_t currTime = 0;
        QueryPerformanceCounter((LARGE_INTEGER *) &currTime);
        _stopTime = currTime;
        _stopped = true;
    }

    void Timer::tick()
    {
        if(_stopped) { _deltaTime = 0.0; return; }

        int64_t currTime = 0;
        QueryPerformanceCounter((LARGE_INTEGER *) &currTime);
        _currTime = currTime;
        _deltaTime = (_currTime - _prevTime) * _secondsPerCount;
        _prevTime = _currTime;
        if(_deltaTime < 0.0) _deltaTime = 0.0;
    }

}