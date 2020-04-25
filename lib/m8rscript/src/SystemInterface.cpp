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
    _heartbeatId = startTimer(_heartOn ? _heartOnTime : _heartrate, false, [this] {
        gpio()->digitalWrite(gpio()->builtinLED(), _heartOn);
        _heartOn = !_heartOn;
        startHeartbeat();
    });
}

void SystemInterface::setHeartrate(Duration rate, Duration ontime)
{
    if (ontime == Duration()) {
        ontime = DefaultHeartOnTime;
    }
    
    if (_heartbeatId < 0) {
        gpio()->digitalWrite(gpio()->builtinLED(), true);
        gpio()->setPinMode(gpio()->builtinLED(), m8r::GPIOInterface::PinMode::Output);
    } else {
        stopTimer(_heartbeatId);
    }

    _heartbeatId = -1;
    _heartrate = rate;
    _heartOnTime = ontime;
    startHeartbeat();
}

