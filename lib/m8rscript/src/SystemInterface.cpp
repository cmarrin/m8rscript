/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "SystemInterface.h"

#include "Application.h"
#include "GPIOInterface.h"
#include "TaskManager.h"

using namespace m8r;

static constexpr Duration DefaultHeartOnTime = 1ms;

SystemInterface* m8r::system()
{
    return Application::system();
}

void SystemInterface::vprintf(ROMString fmt, va_list args) const
{
    print(String::vformat(fmt, args).c_str());
}

bool SystemInterface::runOneIteration()
{
    return taskManager()->runOneIteration();
}

int8_t SystemInterface::startTimer(Duration duration, bool repeat, std::function<void()> cb)
{
    int8_t id = -1;
    
    for (int i = 0; i < NumTimers; ++i) {
        if (!_timers[i].running) {
            id = i;
            break;
        }
    }
    
    if (id < 0) {
        return id;
    }
    
    _timers[id]. running = true;
    
    Thread(512, [this, id, duration, repeat, cb] {
        while (1) {
            {
                Lock lock(_timers[id].mutex);
                if (_timers[id].cond.waitFor(lock, std::chrono::microseconds(duration.us())) != Condition::WaitResult::TimedOut) {
                    // Timer stopped
                    _timers[id].running = false;
                    break;
                }
                
                cb();
                if (repeat) {
                    continue;
                } else {
                    _timers[id].running = false;
                    break;
                }
            }
        }
    }).detach();
    
    return id;
}

void SystemInterface::stopTimer(int8_t id)
{
    if (id < 0 || id >= NumTimers || !_timers[id].running) {
        return;
    }
    
    Lock lock(_timers[id].mutex);
    _timers[id].cond.notify(true);
    _timers[id].running = false;
}

void SystemInterface::startHeartbeat()
{
    if (!_heartbeatTimer) {
        _heartbeatTimer = Timer::create(0s, Timer::Behavior::Once, [this](Timer*) {
            gpio()->digitalWrite(gpio()->builtinLED(), _heartOn);
            _heartOn = !_heartOn;
            _heartbeatTimer->start(_heartOn ? _heartOnTime : _heartrate);
        });
    }
    
    _heartOn = false;
    if (_heartrate) {
        _heartbeatTimer->start(_heartrate);
    }
}

void SystemInterface::setHeartrate(Duration rate, Duration ontime)
{
    if (ontime == Duration()) {
        ontime = _defaultHeartOnTime ?: DefaultHeartOnTime;
    }
    
    if (!_heartbeatTimer) {
        gpio()->digitalWrite(gpio()->builtinLED(), true);
        gpio()->setPinMode(gpio()->builtinLED(), m8r::GPIOInterface::PinMode::Output);
    }

    _heartrate = rate;
    _heartOnTime = ontime;
    startHeartbeat();
}

void SystemInterface::setDefaultHeartOnTime(Duration ontime)
{
    _defaultHeartOnTime = ontime;
    setHeartrate(_heartrate);
}
