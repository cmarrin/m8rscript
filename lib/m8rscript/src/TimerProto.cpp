/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "TimerProto.h"

#include "ExecutionUnit.h"
#include "Timer.h"

using namespace m8rscript;

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
    if (nparams != 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }

    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::MissingThis);
    }
    
    Value func = eu->stack().top();
    
    // Store func so it doesn't get gc'ed
    thisValue.setProperty(SAtom(SA::__object), func, Value::SetType::AddIfNeeded);
    
    Timer* timer = new Timer();
    timer->setCallback([eu, func](Timer*)
    {
        if (func) {
            eu->fireEvent(func, Value(), nullptr, 0);
        }
    });

    thisValue.setProperty(SAtom(SA::__impl), Value(timer), Value::SetType::AddIfNeeded);
    
    // Add a destructor for the timer
    Value dtor([](ExecutionUnit*, Value thisValue, uint32_t nparams)
    {
        Timer* timer = reinterpret_cast<Timer*>(thisValue.property(SAtom(SA::__impl)).asRawPointer());
        if (timer) {
            delete timer;
            thisValue.setProperty(SAtom(SA::__impl), Value(), Value::SetType::AddIfNeeded);
        }
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    });
    
    thisValue.setProperty(SAtom(SA::__destructor), dtor, Value::SetType::AddIfNeeded);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TimerProto::start(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1 || nparams > 2) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }

    Duration duration(eu->stack().top(1 - nparams).toFloatValue(eu));
    
    bool repeating = false;
    
    if (nparams > 1) {
        repeating = eu->stack().top().toBoolValue(eu);
    }
    
    Timer* timer = reinterpret_cast<Timer*>(thisValue.property(SAtom(SA::__impl)).asRawPointer());

    if (!timer) {
        return CallReturnValue(Error::Code::InternalError);
    }

    timer->start(duration, repeating ? Timer::Behavior::Repeating : Timer::Behavior::Once);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TimerProto::stop(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams != 0) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    Timer* timer = reinterpret_cast<Timer*>(thisValue.property(SAtom(SA::__impl)).asRawPointer());

    if (!timer) {
        return CallReturnValue(Error::Code::InternalError);
    }

    timer->stop();

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
