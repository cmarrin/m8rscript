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
    
    PUSHI = 0x30,   // Lower 4 bits is number from -8 to +7
    CALL = 0x40,    // Lower 4 bits is number of params from 0 to 15
    NEW = 0x50,     // Lower 4 bits is number of params from 0 to 15
    
    PREINC = 0x60, PREDEC = 0x61, POSTINC = 0x62, POSTDEC = 0x63, UPLUS = 0x64, UMINUS = 0x65, UNOT = 0x66, UNEG = 0x67,
    LOR = 0x70, LAND = 0x71, AND = 0x72, OR = 0x73, XOR = 0x74, EQ = 0x75, NE = 0x76, LT = 0x77,
    LE = 0x78, GT = 0x79, GE = 0x7A, SHL = 0x7B, SHR = 0x7C, SAR = 0x7D, ADD = 0x7E, SUB = 0x7F,
    MUL = 0x80, DIV = 0x81, MOD = 0x82,
    STO = 0x90, STOMUL = 0x91, STOADD = 0x92, STOSUB = 0x93, STODIV = 0x94, STOMOD = 0x95, STOSHL = 0x96, STOSHR = 0x97,
    STOSAR = 0x98, STOAND = 0x99, STOOR = 0x9A, STOXOR = 0x9B,

    DEREF = 0xA0, NEWID = 0xA1, DEL = 0xA2, END = 0xA3,
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
    
    String stringFromCode(uint32_t nestingLevel, Object* obj) const;

    void setFunction(Function* function) { _currentFunction = function; }
    
    Label label();
    
    void addParam(const Atom& atom) { _currentFunction->addParam(atom); }
    
    void addCode(const char*);
    void addCode(uint32_t);
    void addCode(float);
    void addCode(const Atom&);
    void addCode(Op);
    
    void addCode(Object* obj) { _currentFunction->addObject(obj); }

    void addCallOrNew(bool call, uint32_t nparams);
    void addFixupJump(bool cond, Label&);
    void addJumpAndFixup(Label&);
    
private:
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
    static const char* stringFromOp(Op op);
    void indentCode(String&) const;
    mutable uint32_t _nestingLevel = 0;
#endif
      
    static uint32_t _nextID;
    
    Parser* _parser;
    Function* _currentFunction;
    Program* _currentProgram;
    uint32_t _id = _nextID++;
};
    
}
