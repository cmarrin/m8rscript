/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Timer.h"

#include "Application.h"
#include "GC.h"
#include "MStream.h"
#include "Parser.h"

#include <cassert>

using namespace m8r;

Timer::Timer(Duration duration, Behavior behavior, const Callback& cb)
    : _duration(duration)
    , _cb(cb)
    , _repeating(behavior == Behavior::Repeating)
{
}

void Timer::start(Duration duration)
{
    stop();
    if (duration) {
        _duration = duration;
    }
    
    _timeToFire = Time::now() + _duration;
    DBG_TIMERS("start timer (%p): now=%s, duration=%s, fire=%s",
                this, Time::now().toString().c_str(), _duration.toString().c_str(), _timeToFire.toString().c_str());
    _running = true;
    system()->taskManager()->addTimer(this);
}

void Timer::stop()
{
    DBG_TIMERS("stop timer (%p)", this);
    _running = false;
    system()->taskManager()->removeTimer(this);
}

void Timer::fire()
{
    stop();
    _cb(this);
    if (_repeating) {
        start();
    }
}

#if M8RSCRIPT_SUPPORT == 1
static StaticObject::StaticFunctionProperty _functionProps[] =
{
    { SA::constructor, TimerProto::constructor },
    { SA::start, TimerProto::start },
    { SA::stop, TimerProto::stop },
};

static StaticObject::StaticProperty _props[] =
{
    { SA::Once, Value(static_cast<int32_t>(Timer::Behavior::Once)) },
    { SA::Repeating, Value(static_cast<int32_t>(Timer::Behavior::Repeating)) },
};

TimerProto::TimerProto()
{
    setProperties(_functionProps, sizeof(_functionProps) / sizeof(StaticFunctionProperty));
    setProperties(_props, sizeof(_props) / sizeof(StaticProperty));
}

CallReturnValue TimerProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // duration, {Once/Repeating}, callback
    //          or
    // duration, callback
    
    if (nparams < 2 || nparams > 3) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }

    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Duration duration(eu->stack().top(1 - nparams).toFloatValue(eu));
    
    bool repeating = false;
    Value func;
    
    if (nparams > 2) {
        repeating = eu->stack().top(2 - nparams).toBoolValue(eu);
        func = eu->stack().top(3 - nparams);
    } else {
        func = eu->stack().top(2 - nparams);
    }
    
    // Store func so it doesn't get gc'ed
    thisValue.setProperty(eu, Atom(SA::__object), func, Value::SetType::AddIfNeeded);
    
    std::shared_ptr<Timer> timer = Timer::create(
        duration, 
        repeating ? Timer::Behavior::Repeating : Timer::Behavior::Once, 
        [eu, func](Timer*)
        {
            if (func) {
                eu->fireEvent(func, Value(), nullptr, 0);
            }
        }
    );
    
    obj->setNativeObject(timer);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TimerProto::start(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams != 0) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    std::shared_ptr<Timer> timer = thisValue.isObject() ? thisValue.asObject()->nativeObject<Timer>() : nullptr;
    if (!timer) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }

    timer->start();

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TimerProto::stop(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams != 0) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    std::shared_ptr<Timer> timer = thisValue.isObject() ? thisValue.asObject()->nativeObject<Timer>() : nullptr;
    if (!timer) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }

    timer->stop();

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

#endif
