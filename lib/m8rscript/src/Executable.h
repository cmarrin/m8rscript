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
#include "MString.h"
#include <functional>

namespace m8r {

class Executable
{
public:
    using ConsoleListener = std::function<void(const String& data, KeyAction)>;
    
    Executable() { }
    virtual ~Executable() { }
    
    virtual CallReturnValue execute() = 0;
    virtual bool readyToRun() const { return true; }
    virtual void requestYield() const { }
    virtual void receivedData(const String& data, KeyAction) { }
    
    void printf(ROMString fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    }

    void vprintf(ROMString fmt, va_list args) const
    {
        print(ROMString::vformat(fmt, args).c_str());
    }
    
    void print(const char* s) const;

    void setConsolePrintFunction(const std::function<void(const String&)>& f) { _consolePrintFunction = std::move(f); }
    std::function<void(const String&)> consolePrintFunction() const { return _consolePrintFunction; }

private:
    std::function<void(const String&)> _consolePrintFunction;
};

}
