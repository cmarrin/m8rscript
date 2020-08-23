/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "CallReturnValue.h"
#include "SharedPtr.h"
#include "Timer.h"
#include <functional>

namespace m8r {

class Stream;

class Executable : public Shared
{
public:
    using ConsoleListener = std::function<void(const String& data, KeyAction)>;
    
    Executable();
    virtual ~Executable() { }
    
    virtual bool load(const Stream&) { return false; }
    virtual CallReturnValue execute() = 0;
    virtual bool readyToRun() const { return _delayComplete; }
    virtual void requestYield() const { }
    virtual void receivedData(const String& data, KeyAction) { }
    virtual void gcMark() { }
    virtual const char* runtimeErrorString() const { return "unknown"; }
    virtual const m8r::ParseErrorList* parseErrors() const { return nullptr; }

    void printf(const char* fmt, ...) const;
    void vprintf(const char* fmt, va_list args) const;
    void print(const char* s) const;

    void setConsolePrintFunction(const std::function<void(const String&)>& f) { _consolePrintFunction = std::move(f); }
    std::function<void(const String&)> consolePrintFunction() const { return _consolePrintFunction; }

    void startDelay(Duration);
    

private:
    std::function<void(const String&)> _consolePrintFunction;
    Timer _delayTimer;
    bool _delayComplete = true;
};

}
