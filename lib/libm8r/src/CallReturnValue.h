/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Error.h"
#include "SystemTime.h"

namespace m8r {

class CallReturnValue {
public:
    // Values:
    //
    //      -6M -1      - delay in -ms
    //      0-999       - return count
    //      1000        - function start
    //      1001        - program finished
    //      1002        - program terminated
    //      1003        - wait for event
    //      1004        - continue execution (yield)
    //      1005-1999   - unused
    //      2000...     - error codes
    static constexpr int32_t MaxReturnCount = 999;
    static constexpr int32_t FunctionStartValue = 1000;
    static constexpr int32_t FinishedValue = 1001;
    static constexpr int32_t TerminatedValue = 1002;
    static constexpr int32_t WaitForEventValue = 1003;
    static constexpr int32_t YieldValue = 1004;
    static constexpr int32_t MaxDelay = 6000000;

    static constexpr int32_t ErrorValue = 2000;
    
    enum class Type { ReturnCount = 0, Delay = 1, FunctionStart, Finished, Terminated, WaitForEvent, Yield };
    
    CallReturnValue(Type type = Type::ReturnCount, uint32_t value = 0)
    {
        switch(type) {
            case Type::Delay:
                 assert(value <= MaxDelay);
                 _value = (value == 0) ? -1 : -value;
                 break;
            case Type::ReturnCount: assert(value <= MaxReturnCount); _value = value; break;
             case Type::FunctionStart: _value = FunctionStartValue; break;
            case Type::Finished: _value = FinishedValue; break;
            case Type::Terminated: _value = TerminatedValue; break;
            case Type::WaitForEvent: _value = WaitForEventValue; break;
            case Type::Yield: _value = YieldValue; break;
        }
    }
    
    CallReturnValue(Error error) { _value = ErrorValue + static_cast<int32_t>(error.code()); }
    
    bool isFunctionStart() const { return _value == FunctionStartValue; }
    bool isError() const { return _value >= ErrorValue; }
    bool isFinished() const { return _value == FinishedValue; }
    bool isTerminated() const { return _value == TerminatedValue; }
    bool isWaitForEvent() const { return _value == WaitForEventValue; }
    bool isYield() const { return _value == YieldValue; }
    bool isReturnCount() const { return _value >= 0 && _value <= MaxReturnCount; }
    bool isDelay() const { return _value < 0 && _value >= -MaxDelay; }
    uint32_t returnCount() const { assert(isReturnCount()); return _value; }
    Error error() const
    {
        return isError() ? static_cast<Error::Code>(_value - ErrorValue) : Error::Code::OK;
    }

    Duration delay() const
    {
        assert(isDelay());
        return Duration(static_cast<int64_t>(-_value) * 1000);
    }

private:
    int32_t _value = 0;
};

}
