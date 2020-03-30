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
#include "TaskManager.h"
#include <cstdint>
#include <functional>

namespace m8r {

class Timer : public NativeObject {
public:
    enum class Behavior { Once, Repeating };

    using Callback = std::function<void(Timer*)>;

    Timer() { }
    
    virtual ~Timer()
    {
        stop();
    }

    bool init(Duration duration, Behavior behavior, const Callback& cb)
    {
        _duration = duration;
        _repeating = behavior == Behavior::Repeating;
        _cb = cb;
        return true;
    }

    bool init(Duration duration, const Callback& cb) { return init(duration, Behavior::Once, cb); }

    Duration duration() const { return _duration; }
    bool repeating() const { return _repeating; }
    
    void start();
    void stop();

private:
    Duration _duration = 0_sec;
    bool _repeating = false;
    Thread _thread;
    Callback _cb;
};

class TimerProto : public StaticObject {
public:
    TimerProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue start(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue stop(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
