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

#define SHOW_CODE 1

namespace m8r {

class Parser;
class Function;
class Program;

//  Opcodes with params have bit patterns.
//  Upper 2 bits are 00
//  The lower 2 bits indicate the number of additional bytes:
//      00 - 1
//      01 - 2
//      10 - unused
//      11 - 4
//
//  The next 4 bits is the opcode class:
//      0000 - unused
//      0001 - PUSHID
//      0010 - PUSHF
//      0011 - PUSHI
//      0100 - PUSHS
//      0101 - JMP
//      0110 - JT
//      0111 - JF
//      1000 - CALL
//      1001 - NEW
//      1010 - PUSHO
//      1011 - RET
//
enum class Op {
    PUSHID = 0x05,   // 0000 0101 - Next 2 bytes are atom
    PUSHF  = 0x0B,   // 0000 1011 - Next 4 bytes are number
    PUSHIX = 0x0C,   // 0000 1100 - Next byte is number
    PUSHSX = 0x10,   // 0001 0000 - Next byte is length from 0 to 255, followed by string (includes trailing '\0')
    
    // The jump instructions use the LSB to indicate the jump type. 0 - next byte is jump address (-128..127), 1 - next 2 bytes are address (HI/LO, -32768..32767)
    JMP = 0x14,     // 0001 0100
    JT = 0x18,      // 0001 1000
    JF = 0x1C,      // 0001 1100
    
    CALLX = 0x20,   // 0010 0000
    NEWX = 0x24,    // 0010 0100
    PUSHO = 0x2B,   // 0010 1011
    RETX = 0x2C,    // 0010 1100
    
    PUSHI = 0x40,   // Lower 4 bits is number from 0 to 15
    CALL = 0x50,    // Lower 4 bits is number of params from 0 to 15
    NEW = 0x60,     // Lower 4 bits is number of params from 0 to 15
    RET = 0x70,     // Lower 4 bits is number of return values from 0 to 15
    
    STO = 0x80, STOMUL = 0x81, STOADD = 0x82, STOSUB = 0x83, STODIV = 0x84, STOMOD = 0x85, STOSHL = 0x86, STOSHR = 0x87,
    STOSAR = 0x88, STOAND = 0x89, STOOR = 0x8A, STOXOR = 0x8B,
    LOR = 0x90, LAND = 0x91, AND = 0x92, OR = 0x93, XOR = 0x94, EQ = 0x95, NE = 0x96, LT = 0x97,
    LE = 0x98, GT = 0x99, GE = 0x9A, SHL = 0x9B, SHR = 0x9C, SAR = 0x9D, ADD = 0x9E, SUB = 0x9F,
    MUL = 0xA0, DIV = 0xA1, MOD = 0xA2,
    PREINC = 0xB0, PREDEC = 0xB1, POSTINC = 0xB2, POSTDEC = 0xB3, UPLUS = 0xB4, UMINUS = 0xB5, UNOT = 0xB6, UNEG = 0xB7,

    DEREF = 0xC0, NEWID = 0xC1, DEL = 0xC2, END = 0xC3,
};

struct Label {
    int32_t label : 20;
    uint32_t uniqueID : 12;
    int32_t fixupAddr : 20;
};

class ExecutionUnit {
public:
    ExecutionUnit(Parser* parser, Program* program)
        : _parser(parser)
        , _currentProgram(program)
    { }
    
    String toString() const;

    void setFunction(Function* function) { _currentFunction = function; }
    
    Label label();
    
    void addParam(const Atom& atom) { _currentFunction->addParam(atom); }
    
    void addCode(const char*);
    void addCode(uint32_t);
    void addCode(float);
    void addCode(const Atom&);
    void addCode(Op);
    void addCode(const ObjectId&);
    
    void addObject(Object* obj) { addCode(_currentProgram->addObject(obj)); }
    void addNamedFunction(Function*, const Atom& name);

    void addCodeWithCount(Op value, uint32_t nparams);
    void addFixupJump(bool cond, Label&);
    void addJumpAndFixup(Label&);
    
private:
    String generateCodeString(uint32_t nestingLevel, const char* functionName, Object* obj) const;

    Op maskOp(Op op, uint8_t mask) const { return static_cast<Op>(static_cast<uint8_t>(op) & ~mask); }
    int8_t intFromOp(Op op, uint8_t mask) const
    {
        uint8_t num = static_cast<uint8_t>(op) & mask;
        if (num & 0x8) {
            num |= 0xf0;
        }
        return static_cast<int8_t>(num);
    }
    uint8_t uintFromOp(Op op, uint8_t mask) const { return static_cast<uint8_t>(op) & mask; }

    int32_t intFromCode(Object* obj, uint32_t index, uint32_t size) const
    {
        uint32_t num = uintFromCode(obj, index, size);
        uint32_t mask = 0x80 << (8 * (size - 1));
        if (num & mask) {
            return num | ~(mask - 1);
        }
        return static_cast<int32_t>(num);
    }
    
    uint32_t uintFromCode(Object* obj, uint32_t index, uint32_t size) const
    {
        uint32_t value = 0;
        for (int i = 0; i < size; ++i) {
            value <<= 8;
            value |= obj->codeAtIndex(index + i);
        }
        return value;
    }
    
#if SHOW_CODE
    struct Annotation {
        uint32_t addr;
        uint32_t uniqueID;
    };
    typedef Vector<Annotation> Annotations;

    uint32_t findAnnotation(uint32_t addr) const;
    void preamble(String& s, uint32_t addr) const;
    static const char* stringFromOp(Op op);
    void indentCode(String&) const;
    mutable uint32_t _nestingLevel = 0;
    mutable Annotations annotations;
#endif
      
    static uint32_t _nextID;
    
    Parser* _parser;
    Function* _currentFunction;
    Program* _currentProgram;
    uint32_t _id = _nextID++;
};
    
}
