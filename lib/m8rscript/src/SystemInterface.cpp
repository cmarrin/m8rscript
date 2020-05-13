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

void SystemInterface::runOneIteration()
{
    taskManager()->runOneIteration();
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
        ontime = DefaultHeartOnTime;
    }
    
    if (!_heartbeatTimer) {
        gpio()->digitalWrite(gpio()->builtinLED(), true);
        gpio()->setPinMode(gpio()->builtinLED(), m8r::GPIOInterface::PinMode::Output);
    }

    _heartrate = rate;
    _heartOnTime = ontime;
    startHeartbeat();
}

