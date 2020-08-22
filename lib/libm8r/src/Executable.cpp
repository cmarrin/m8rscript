/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Executable.h"

#include "SystemInterface.h"

using namespace m8r;

Executable::Executable()
{
    _delayTimer.setCallback([this](Timer*) {
        _delayComplete = true;
    });
}

void Executable::print(const char* s) const
{
    if (_consolePrintFunction) {
        _consolePrintFunction(s);
    } else {
        system()->print(s);
    }
}

void Executable::printf(const char* fmt, ...) const
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
}

void Executable::vprintf(const char* fmt, va_list args) const
{
    print(String::vformat(fmt, args).c_str());
}

void Executable::startDelay(Duration duration)
{
    _delayComplete = false;
    _delayTimer.start(duration);
}

