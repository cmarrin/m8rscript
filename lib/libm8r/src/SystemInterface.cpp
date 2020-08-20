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

SystemInterface::SystemInterface()
{
    _heartbeatTimer.setCallback([this](Timer*) {
        gpio()->digitalWrite(gpio()->builtinLED(), _heartOn);
        _heartOn = !_heartOn;
        startHeartbeat();
    });
}

bool SystemInterface::runOneIteration()
{
    return taskManager()->runOneIteration();
}

void SystemInterface::startHeartbeat()
{
    _heartbeatTimer.start(_heartOn ? _heartOnTime : _heartrate);
}

void SystemInterface::setHeartrate(Duration rate, Duration ontime)
{
    if (ontime == Duration()) {
        ontime = _defaultHeartOnTime ?: DefaultHeartOnTime;
    }
    
    if (!_heartbeatTimer.running()) {
        gpio()->digitalWrite(gpio()->builtinLED(), true);
        gpio()->setPinMode(gpio()->builtinLED(), m8r::GPIOInterface::PinMode::Output);
    } else {
        _heartbeatTimer.stop();
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

void SystemInterface::registerScriptingLanguage(const ScriptingLanguage* lang)
{
    _scriptingLanguages.push_back(lang);
}
