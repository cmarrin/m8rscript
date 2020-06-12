/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2020, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Defines.h"
#include "Object.h"
#include "SystemTime.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace m8r {

class Timer : public NativeObject {
public:
    enum class Behavior { Once, Repeating };

    using Callback = std::function<void(Timer*)>;
    
    Timer(Duration duration, Behavior behavior, const Callback& cb);

    virtual ~Timer()
    {
        stop();
    }

    static std::shared_ptr<Timer> create(Duration duration, Behavior behavior, const Callback& cb)
    {
        return std::make_shared<Timer>(duration, behavior, cb);
    }

    Duration duration() const { return _duration; }
    bool repeating() const { return _repeating; }
    Time timeToFire() const { return _timeToFire; }
    
    void setDuration(Duration d)
    {
        if (running()) {
            return;
        }
        _duration = d;
    }

    void start(Duration = Duration());
    void stop();
    
    bool running() const { return _running; }
    
    void fire();

private:
    Duration _duration = 0s;
    Time _timeToFire; // Set when Timer is started
    Callback _cb;
    bool _repeating = false;
    bool _running = false;
};

#if SCRIPT_SUPPORT == 1
class TimerProto : public StaticObject {
public:
    TimerProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue start(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue stop(ExecutionUnit*, Value thisValue, uint32_t nparams);
};
#endif

}
