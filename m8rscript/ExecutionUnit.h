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
#include "Opcodes.h"

#define SHOW_CODE 1

namespace m8r {

class Parser;
class Function;
class Program;
class Printer;

class ExecutionStack : public Stack<Value>, public Object
{
public:
    ExecutionStack(uint32_t size) : Stack<Value>(size) { }

    virtual const char* typeName() const override { return "Local"; }

    virtual Value elementRef(int32_t index) override { return Value(this, index, false); }
    virtual const Value element(uint32_t index) const override { return inFrame(index); }
    virtual bool setElement(uint32_t index, const Value& value) override { inFrame(index) = value; return true; }
    virtual bool appendElement(const Value&) override { assert(0); return false; }
    virtual size_t elementCount() const override { assert(0); return 0; }
    virtual void setElementCount(size_t) override { assert(0); }
};

class ExecutionUnit {
public:
    ExecutionUnit(Printer* printer = nullptr) : _stack(10), _printer(printer) { }
    
    void run(Program* program);
    
    m8r::String generateCodeString(const Program* program) const;
    
private:
    bool printError(const char* s) const;
    
    Value* valueFromId(Atom, const Object*) const;
    uint32_t call(Program* program, uint32_t nparams, Object*, bool isNew);
    Value deref(Program*, Object*, const Value&);
    bool deref(Program*, Value&, const Value&);
    Atom propertyNameFromValue(Program*, const Value&);

    m8r::String generateCodeString(const Program*, const Object*, const char* functionName, uint32_t nestingLevel) const;

    static Op maskOp(Op op, uint8_t mask) { return static_cast<Op>(static_cast<uint8_t>(op) & ~mask); }
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
        for (int i = 0; i < size; ++i) {
            value <<= 8;
            value |= code[index + i];
        }
        return value;
    }
    
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

    
#if SHOW_CODE
    struct Annotation {
        uint32_t addr;
        uint32_t uniqueID;
    };
    typedef Vector<Annotation> Annotations;

    uint32_t findAnnotation(uint32_t addr) const;
    void preamble(m8r::String& s, uint32_t addr) const;
    static const char* stringFromOp(Op op);
    void indentCode(m8r::String&) const;
    mutable uint32_t _nestingLevel = 0;
    mutable Annotations annotations;
#endif
      
    ExecutionStack _stack;

    mutable uint32_t _nerrors = 0;
    Printer* _printer;
};
    
}
