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
#include "Task.h"

namespace m8rscript {

class Parser;

using ExecutionStack = m8r::Stack<Value>;

class ExecutionUnit : public m8r::Executable {
public:
    static m8r::MemoryType memoryType() { return m8r::MemoryType::ExecutionUnit; }

    ExecutionUnit();
    ~ExecutionUnit();
    
    virtual void gcMark() override;

    // Executable overrides
    virtual bool load(const m8r::Stream&) override;
    virtual const char* runtimeErrorString() const override { return _errorString.c_str(); }
    virtual const m8r::ParseErrorList* parseErrors() const override { return &_parseErrorList; }

    virtual m8r::CallReturnValue execute() override;
    virtual bool readyToRun() const override { return !_eventQueue.empty() || !executingDelay(); }
    virtual void requestYield() const override { _yield = true; _checkForExceptions = true; }
    virtual void receivedData(const m8r::String& data, m8r::KeyAction) override;

    void startExecution(m8r::Mad<Program>);
    
    void startDelay(m8r::Duration);
    void continueDelay();
    
    m8r::CallReturnValue import(const m8r::Stream&, Value);
    
    ExecutionStack& stack() { return _stack; }

    void requestTermination() { _terminate = true; }
    
    const m8r::Mad<Program> program() const { return _program; }
    
    uint32_t argumentCount() const { return _actualParamCount; }
    Value& argument(int32_t i) { return _stack.inFrame(i); }
    
    void fireEvent(const Value& func, const Value& thisValue, const Value* args, int32_t nargs);
    
    void setConsoleListener(Value func)
    {
        if (_program.valid()) {
            _program->setProperty(SAtom(SA::consoleListener), func, Value::Value::SetType::AddIfNeeded);
        }
    }

    void startEventListening() { _numEventListeners++; }
    void stopEventListening() { _numEventListeners--; }

    uint32_t upValueStackIndex(uint32_t index, uint16_t frame) const;
    void addOpenUpValue(const m8r::SharedPtr<UpValue>& upValue) { _openUpValues.push_back(upValue); }
    
    m8r::Mad<Callable> currentFunction() const { return _function; }
    
    uint32_t lineno() const { return _lineno; }

    m8r::String debugString(uint16_t index);

    static m8r::Mad<m8r::String> createString(const m8r::String& other);
    static m8r::Mad<m8r::String> createString(m8r::String&& other);
    static m8r::Mad<m8r::String> createString(const char* str, int32_t length = -1);
    
    void requestTerminate() const { _terminate = true; _checkForExceptions = true; }

    void startFunction(m8r::Mad<Object> function, m8r::Mad<Object> thisObject, uint32_t nparams);

private:
    static constexpr uint32_t MaxRunTimeErrrors = 30;
    static constexpr uint32_t DelayThreadSize = 1024;
    
    Op checkForExceptions(uint8_t& imm)
    {
        _checkForExceptions = false;
        if (_terminate) {
            _yield = false;
            return Op::END;
        }
        if (_yield) {
            _yield = false;
            return Op::YIELD;
        }
        if (!_eventQueue.empty() && !_executingEvent) {
            return Op::YIELD;
        }
        return opFromCode(_currentAddr, imm);
    }
    
    Op dispatchNextOp(uint8_t& imm)
    {
        return _checkForExceptions ? checkForExceptions(imm) : opFromCode(_currentAddr, imm);
    }
    
    m8r::CallReturnValue endFunction();
    m8r::CallReturnValue runNextEvent();

    void printError(const char* s, ...) const;
    void printError(m8r::Error) const;
    
    Value* valueFromId(m8r::Atom, const Object*) const;

    void updateCodePointer()
    {
        _code = _function.valid() ? &(_function->code()->front()) : nullptr;
        _currentAddr = _code;
    }
    
    Value derefId(m8r::Atom);
    void stoIdRef(m8r::Atom, const Value&);
    
    void setInFrame(uint32_t r, const Value& v)
    {
        assert(r <= MaxRegister);
        if (r >= _formalParamCount) {
            r += _localOffset;
        }
        _stack.atFrame(r) = v;
    }
    
    void closeUpValues(uint32_t frame);
    
    Value& reg(uint32_t r)
    {
        assert(r <= MaxRegister);
        if (r >= _formalParamCount) {
            r += _localOffset;
        }
        return _stack.atFrame(r);
    }
    
    
    const Value regOrConst()
    {
        uint32_t r = byteFromCode(_currentAddr);
        if (r <= MaxRegister) {
            return reg(r);
        }

        if (shortSharedAtomConstant(r)) {
            return Value(m8r::Atom(byteFromCode(_currentAddr)));
        } else if (longSharedAtomConstant(r)) {
            return Value(m8r::Atom(uNFromCode(_currentAddr)));
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
            return !_delayComplete;
        }
        return (_callRecords.empty() ? true : _callRecords.back()._executingDelay) && !_delayComplete;
    }
    
    void checkEventQueueConsistency()
    {
        uint32_t index = 0;
        while (index < _eventQueue.size()) {
            assert(_eventQueue.size() - index >= 3);
            assert(_eventQueue[index + 2].isInteger());
            int32_t nargs = _eventQueue[index + 2].asIntValue();
            assert(static_cast<int32_t>(_eventQueue.size()) >= nargs + static_cast<int32_t>(index) + 3);
            index += nargs + 3;
        }
        assert(index == _eventQueue.size());
    }
    
    struct CallRecord {
        CallRecord() { }
        CallRecord(uint32_t pc, uint32_t frame, m8r::Mad<Object> func, m8r::Mad<Object> thisObj, uint32_t paramCount, uint32_t lineno, uint32_t localsAdded)
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
        m8r::Mad<Object> _func;
        m8r::Mad<Object> _thisObj;
        uint32_t _lineno = 0;
        uint32_t _localsAdded = 0;
        bool _executingDelay = false;
    };
    
    using EventValue = Value;
    using CallRecordVector = m8r::Vector<CallRecord>;
    using EventValueVector = m8r::Vector<EventValue>;

    CallRecordVector _callRecords;
    ExecutionStack _stack;
    
    m8r::Mad<Program> _program;
    m8r::Mad<Object> _function;
    m8r::Mad<Object> _this;
    uint32_t _localOffset = 0;
    uint16_t _formalParamCount = 0;
    uint32_t _actualParamCount = 0;

    const uint8_t* _code = nullptr;
    const uint8_t* _currentAddr = nullptr;
    
    mutable uint32_t _nerrors = 0;
    
    EventValueVector _eventQueue;

    bool _executingEvent = false;
    bool _delayComplete = true;
    mutable bool _checkForExceptions = false;
    mutable bool _terminate = false;
    mutable bool _yield = false;
    
    uint32_t _numEventListeners = 0;
    
    uint32_t _lineno = 0;
    
    m8r::Vector<m8r::SharedPtr<UpValue>> _openUpValues;
    
    Value _consoleListener;
    
    m8r::Timer _delayTimer;
    
    m8r::String _errorString;
    m8r::ParseErrorList _parseErrorList;
};

}
