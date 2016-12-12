/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#pragma once

#include <cstdint>

#include "Atom.h"
#include "Function.h"
#include "Program.h"

namespace m8r {

class Parser;
class Function;
class Program;
class SystemInterface;

class ExecutionStack : public Stack<Value>, public Object
{
public:
    ExecutionStack(uint32_t size) : Stack<Value>(size) { }

    virtual const char* typeName() const override { return "Local"; }

    virtual Value elementRef(int32_t index) override { return Value(Program::stackId(), index, false); }
    virtual const Value element(uint32_t index) const override { return inFrame(index); }
    virtual bool setElement(ExecutionUnit*, uint32_t index, const Value& value) override { inFrame(index) = value; return true; }
    virtual bool appendElement(ExecutionUnit*, const Value&) override { assert(0); return false; }
    virtual size_t elementCount() const override { assert(0); return 0; }
    virtual void setElementCount(size_t) override { assert(0); }
    
    void popBaked(ExecutionUnit* eu, Value& value)
    {
        pop(value);
        if (value.canBeBaked()) {
            value = value.bake(eu);
        }
    }
    
    void topBaked(ExecutionUnit* eu, Value& value)
    {
        std::swap(value, top());
        if (value.canBeBaked()) {
            value = value.bake(eu);
        }
    }

    Value* framePtr() { return Stack::framePtr(); }

protected:
    virtual bool serialize(Stream*, Error&, Program*) const override
    {
        return true;
    }

    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override
    {
        return true;
    }
};

class ExecutionUnit {
public:
    friend class Function;
    
    ExecutionUnit(SystemInterface* system = nullptr) : _stack(200), _system(system) { }
    
    void startExecution(Program*);
    
    // Return -1 when finished. Otherwise return value is number of milliseconds to delay before calling again
    int32_t continueExecution();
    
    ExecutionStack& stack() { return _stack; }

    void requestTermination() { _terminate = true; }
    
    SystemInterface* system() const { return _system; }
    Program* program() const { return _program; }
    
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
    
private:
    void startFunction(ObjectId function, uint32_t nparams);

    bool printError(const char* s, ...) const;
    bool checkTooManyErrors() const;
    
    Value* valueFromId(Atom, const Object*) const;

    m8r::String generateCodeString(const Program*, const Object*, const char* functionName, uint32_t nestingLevel) const;

    void updateCodePointer()
    {
        assert(_object);
        assert(_functionPtr->code());
        _codeSize = _functionPtr->code()->size();
        assert(_codeSize);
        _code = &(_functionPtr->code()->at(0));
    }
    
    Value derefId(Atom);
    
    const Value& regOrConst(uint32_t r) const { return (r > MaxRegister) ? _constantsPtr[r - MaxRegister] : _framePtr[r]; }
    Value& regOrConst(uint32_t r) { return (r > MaxRegister) ? _constantsPtr[r - MaxRegister] : _framePtr[r]; }

    uint32_t _pc = 0;
    Program* _program = nullptr;
    ObjectId _object;
    Function* _functionPtr;
    Value* _constantsPtr;
    Value* _framePtr;
    
    const Instruction* _code;
    size_t _codeSize;
    
    ExecutionStack _stack;
    mutable uint32_t _nerrors = 0;
    SystemInterface* _system;
    mutable bool _terminate = false;
};
    
}
