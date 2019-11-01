/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Heartbeat.h"

#include "GPIOInterface.h"

using namespace m8r;

Heartbeat::Heartbeat()
    : _task([this]() {
        system()->gpio()->digitalWrite(system()->gpio()->builtinLED(), !_upbeat);
        _upbeat = !_upbeat;
        return CallReturnValue(CallReturnValue::Type::MsDelay, _upbeat ? (HeartrateMs - DownbeatMs) : DownbeatMs);
    })
{
    system()->gpio()->setPinMode(system()->gpio()->builtinLED(), GPIOInterface::PinMode::Output);
}
