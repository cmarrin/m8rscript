/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2020, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
#include "SystemInterface.h"
#include "Task.h"
#include <cstdint>
#include <functional>

namespace m8r {

class Timer : public NativeObject, public TaskBase {
public:
    enum class Behavior { Once, Repeating };

    Timer() { }
    
    virtual ~Timer() { }

    bool init(Duration duration, Behavior behavior, const FinishCallback& cb)
    {
        _duration = duration;
        _repeating = behavior == Behavior::Repeating;
        _cb = cb;

#ifndef NDEBUG
        _name = String::format("Timer:%s %s(%p)", _duration.toString().c_str(), _repeating ? "repeating" : "once", this);
#endif
        return true;
    }
    
    bool init(Duration duration, const FinishCallback& cb) { return init(duration, Behavior::Once, cb); }
    
    virtual Duration duration() const override { return _duration; }
    bool repeating() const { return _repeating; }
    
    void start();
    void stop();
    
    virtual void finish() { if (_cb) _cb(this); }

private:
    virtual CallReturnValue execute() override { return CallReturnValue(CallReturnValue::Type::Finished); }
    
    Duration _duration = 0_sec;
    bool _repeating = false;
    FinishCallback _cb;
};

class TimerProto : public StaticObject {
public:
    TimerProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue start(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue stop(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
