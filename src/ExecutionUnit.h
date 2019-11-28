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
#include "Telnet.h"

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
    
    CallReturnValue import(const Stream&, Value);
    
    ExecutionStack& stack() { return _stack; }

    void requestTermination() { _terminate = true; }
    
    const Mad<Program> program() const { return _program; }
    
    static uint8_t byteFromInt(uint64_t value, uint32_t index)
    {
        assert(index < 8);
        return static_cast<uint8_t>(value >> (8 * index));
    }
    
    static Op maskOp(Op op, uint8_t mask) { return static_cast<Op>(static_cast<uint8_t>(op) & ~mask); }

    static uint32_t sizeFromOp(Op op)
    {
        uint32_t size = static_cast<uint8_t>(op) & 0x03;
        if (size < 2) {
            return size + 1;
        }
        if (size == 2) {
            return 4;
        }
        return 8;
    }
    
    static int8_t intFromOp(Op op, uint8_t mask)
    {
        uint8_t num = static_cast<uint8_t>(op) & mask;
        if (num & 0x8) {
            num |= 0xf0;
        }
        return static_cast<int8_t>(num);
    }
    static uint8_t uintFromOp(Op op, uint8_t mask) { return static_cast<uint8_t>(op) & mask; }

    static int32_t intFromCode(const uint8_t* code, uint32_t index, uint32_t size)
    {
        uint32_t num = uintFromCode(code, index, size);
        uint32_t mask = 0x80 << (8 * (size - 1));
        if (num & mask) {
            return num | ~(mask - 1);
        }
        return static_cast<int32_t>(num);
    }
    
    static uint32_t uintFromCode(const uint8_t* code, uint32_t index, uint32_t size)
    {
        uint32_t value = 0;
        for (uint32_t i = 0; i < size; ++i) {
            value <<= 8;
            value |= code[index + i];
        }
        return value;
    }
    
    uint32_t argumentCount() const { return _actualParamCount; }
    Value& argument(int32_t i) { return _stack.inFrame(i); }
    
    void fireEvent(const Value& func, const Value& thisValue, const Value* args, int32_t nargs);

    void receivedData(const String&, Telnet::Action);

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
        _program->setProperty(Atom(SA::consoleListener), func, Value::SetPropertyType::AddIfNeeded);
    }

    void startEventListening() { _numEventListeners++; }
    void stopEventListening() { _numEventListeners--; }

    uint32_t upValueStackIndex(uint32_t index, uint16_t frame) const;
    void addOpenUpValue(const Mad<UpValue> upValue) { _openUpValues.push_back(upValue); }
    
    Mad<Object> currentFunction() const { return _function; }
    
    uint32_t lineno() const { return _lineno; }

    String debugString(uint16_t index);

private:
    static constexpr uint32_t MaxRunTimeErrrors = 30;
    
    Op dispatchNextOp(Instruction& inst, uint16_t& checkCounter)
    {
        if (_nerrors > MaxRunTimeErrrors) {
            tooManyErrors();
            _terminate = true;
            return Op::END;
        }
        if ((++checkCounter & 0xff) == 0) {
            if (_terminate) {
                return Op::END;
            }
            GC::gc();
            if (checkCounter == 0) {
                return Op::YIELD;
            }
        }
        inst = _code[_pc++];
        return inst.op();
    }

    void startFunction(Mad<Object> function, Mad<Object> thisObject, uint32_t nparams);
    CallReturnValue runNextEvent();

    void printError(ROMString s, ...) const;
    void printError(CallReturnValue::Error) const;
    void tooManyErrors() const;
    
    Value* valueFromId(Atom, const Object*) const;

    m8r::String generateCodeString(const Mad<Program>, const Mad<Object>, const char* functionName, uint32_t nestingLevel) const;

    void updateCodePointer()
    {
        assert(_function->code());
        _codeSize = _function->code()->size();
        _code = &(_function->code()->front());
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
     
    const Value& regOrConst(uint32_t r)
    {
        if (r > MaxRegister) {
            return _constants[r - MaxRegister - 1];
        }
        if (r >= _formalParamCount) {
            return _framePtr[r + _localOffset];
        }
        return _framePtr[r];
    }
    
    bool isConstant(uint32_t r) { return r > MaxRegister; }

    int compareValues(const Value& a, const Value& b);
    
    struct CallRecord {
        CallRecord() { }
        CallRecord(uint32_t pc, uint32_t frame, Mad<Object> func, Mad<Object> thisObj, uint32_t paramCount, uint32_t lineno)
            : _pc(pc)
            , _paramCount(paramCount)
            , _frame(frame)
            , _func(func)
            , _thisObj(thisObj)
            , _lineno(lineno)
        { }
        
        uint32_t _pc : 23;
        uint32_t _paramCount : 8;
        uint32_t _frame;
        Mad<Object> _func;
        Mad<Object> _thisObj;
        uint32_t _lineno;
    };
    
    using EventValue = Value;
    using CallRecordVector = Vector<CallRecord>;
    using EventValueVector = Vector<EventValue>;

    CallRecordVector _callRecords;
    ExecutionStack _stack;
    
    uint32_t _pc = 0;
    Mad<Program> _program;
    Mad<Object> _function;
    Mad<Object> _this;
    const Value* _constants = nullptr;
    Value* _framePtr = nullptr;
    uint32_t _localOffset = 0;
    uint32_t _formalParamCount = 0;
    uint32_t _actualParamCount = 0;

    const Instruction* _code = nullptr;
    size_t _codeSize;
    
    mutable uint32_t _nerrors = 0;
    mutable bool _terminate = false;
    
    EventValueVector _eventQueue;
    bool _executingEvent = false;
    uint32_t _numEventListeners = 0;
    
    uint32_t _lineno = 0;
    
    Vector<Mad<UpValue>> _openUpValues;
    
    std::function<void(const String&)> _consolePrintFunction;
    Value _consoleListener;
};

}
