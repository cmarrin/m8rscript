/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cstdint>

#include "Atom.h"
#include "Closure.h"
#include "Program.h"

namespace m8r {

class Parser;
class Function;
class Program;

using ExecutionStack = Stack<Value>;

class ExecutionUnit {
public:
    friend class Closure;
    friend class Function;
    
    static MemoryType memoryType() { return MemoryType::ExecutionUnit; }

    ExecutionUnit() : _stack(20) { }
    ~ExecutionUnit() { }
    
    void gcMark();

    void startExecution(Mad<Program>);
    
    CallReturnValue continueExecution();

    void startDelay(Duration);
    void continueDelay();
    
    CallReturnValue import(const Stream&, Value);
    
    ExecutionStack& stack() { return _stack; }

    void requestTermination() { _terminate = true; }
    
    const Mad<Program> program() const { return _program; }
    
    uint32_t argumentCount() const { return _actualParamCount; }
    Value& argument(int32_t i) { return _stack.inFrame(i); }
    
    void fireEvent(const Value& func, const Value& thisValue, const Value* args, int32_t nargs);

    void receivedData(const String&, KeyAction);

    void setConsolePrintFunction(std::function<void(const String&)> f) { _consolePrintFunction = f; }
    const std::function<void(const String&)>& consolePrintFunction() const { return _consolePrintFunction; }
    
    void vprintf(ROMString, va_list) const;

    void printf(ROMString fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    }
    
    void print(const char* s) const;

    void setConsoleListener(Value func)
    {
        if (_program.valid()) {
            _program->setProperty(Atom(SA::consoleListener), func, Value::SetPropertyType::AddIfNeeded);
        }
    }

    void startEventListening() { _numEventListeners++; }
    void stopEventListening() { _numEventListeners--; }

    uint32_t upValueStackIndex(uint32_t index, uint16_t frame) const;
    void addOpenUpValue(const Mad<UpValue> upValue) { _openUpValues.push_back(upValue); }
    
    Mad<Callable> currentFunction() const { return _function; }
    
    uint32_t lineno() const { return _lineno; }

    String debugString(uint16_t index);

    static Mad<String> createString(const String& other);
    static Mad<String> createString(String&& other);
    static Mad<String> createString(const char* str, int32_t length = -1);
    
    bool readyToRun() const
    {
        return !_eventQueue.empty() || (executingDelay() && _delayComplete);
    }

private:
    static constexpr uint32_t MaxRunTimeErrrors = 30;
    static constexpr uint32_t DelayThreadSize = 1024;
    
    Op dispatchNextOp(uint8_t& imm)
    {
        if (_nerrors > MaxRunTimeErrrors) {
            tooManyErrors();
            _terminate = true;
            return Op::END;
        }
        if (_terminate) {
            return Op::END;
        }
        if (!_eventQueue.empty()) {
            return Op::YIELD;
        }
        if (++_yieldCounter == 0) {
            return Op::YIELD;
        }
        return opFromCode(_currentAddr, imm);
    }
    
    void startFunction(Mad<Object> function, Mad<Object> thisObject, uint32_t nparams);
    CallReturnValue endFunction();
    CallReturnValue runNextEvent();

    void printError(ROMString s, ...) const;
    void printError(CallReturnValue::Error) const;
    void tooManyErrors() const;
    
    Value* valueFromId(Atom, const Object*) const;

    m8r::String generateCodeString(const Mad<Program>, const Mad<Object>, const char* functionName, uint32_t nestingLevel) const;

    void updateCodePointer()
    {
        _code = _function.valid() ? &(_function->code()->front()) : nullptr;
        _currentAddr = _code;
    }
    
    Value derefId(Atom);
    void stoIdRef(Atom, const Value&);
    
    void setInFrame(uint32_t r, const Value& v)
    {
        assert(r <= MaxRegister);
        if (r >= _formalParamCount) {
            _framePtr[r + _localOffset] = v;
        } else {
            _framePtr[r] = v;
        }
    }
    
    void closeUpValues(uint32_t frame);
    
    Value& reg(uint32_t r)
    {
        assert(r <= MaxRegister);
        if (r >= _formalParamCount) {
            return _framePtr[r + _localOffset];
        }
        return _framePtr[r];
    }
    
    
    const Value regOrConst()
    {
        uint32_t r = byteFromCode(_currentAddr);
        if (r <= MaxRegister) {
            return reg(r);
        }

        if (shortSharedAtomConstant(r)) {
            return Value(Atom(byteFromCode(_currentAddr)));
        } else if (longSharedAtomConstant(r)) {
            return Value(Atom(uNFromCode(_currentAddr)));
        } else {
            Value value;
            _function->constant(r, value);
            return value;
        }
    }
    
    bool isConstant(uint32_t r) { return r > MaxRegister; }

    int compareValues(const Value& a, const Value& b);
    
    bool executingDelay() const
    {
        if (_callRecords.empty()) {
            return false;
        }
        return _callRecords.back()._executingDelay;
    }
    
    struct CallRecord {
        CallRecord() { }
        CallRecord(uint32_t pc, uint32_t frame, Mad<Object> func, Mad<Object> thisObj, uint32_t paramCount, uint32_t lineno, uint32_t localsAdded)
            : _pc(pc)
            , _paramCount(paramCount)
            , _frame(frame)
            , _func(func)
            , _thisObj(thisObj)
            , _lineno(lineno)
            , _localsAdded(localsAdded)
        { }
        
        uint32_t _pc : 23;
        uint32_t _paramCount : 8;
        uint32_t _frame;
        Mad<Object> _func;
        Mad<Object> _thisObj;
        uint32_t _lineno = 0;
        uint32_t _localsAdded = 0;
        bool _executingDelay = false;
    };
    
    using EventValue = Value;
    using CallRecordVector = Vector<CallRecord>;
    using EventValueVector = Vector<EventValue>;

    CallRecordVector _callRecords;
    ExecutionStack _stack;
    
    Mad<Program> _program;
    Mad<Object> _function;
    Mad<Object> _this;
    uint32_t _localOffset = 0;
    uint16_t _formalParamCount = 0;
    uint32_t _actualParamCount = 0;

    const uint8_t* _code = nullptr;
    const uint8_t* _currentAddr = nullptr;
    Value* _framePtr = nullptr;
    
    mutable uint32_t _nerrors = 0;
    mutable bool _terminate = false;
    
    EventValueVector _eventQueue;
    bool _executingEvent = false;
    bool _delayComplete = false;
    uint8_t _yieldCounter = 0;
    uint32_t _numEventListeners = 0;
    
    uint32_t _lineno = 0;
    
    Vector<Mad<UpValue>> _openUpValues;
    
    std::function<void(const String&)> _consolePrintFunction;
    Value _consoleListener;
};

}
