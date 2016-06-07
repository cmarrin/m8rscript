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
#include "Object.h"

#define SHOW_CODE 1

namespace m8r {

class Parser;

enum class Op {
    PUSHID = 0x01,  // Next 2 bytes are atom
    PUSHF = 0x02,   // Next 4 bytes are number
    PUSHIX1 = 0x03,  // Next byte is number
    PUSHIX2 = 0x04,  // Next 2 bytes are number
    PUSHIX4 = 0x05,  // Next 4 bytes are number
    PUSHSX1 = 0x06, // Next byte is length from 0 to 255, followed by string (includes trailing '\0')
    PUSHSX2 = 0x07, // Next 2 bytes are length from 0 to 64435, followed by string (includes trailing '\0')
    
    // The jump instructions use the LSB to indicate the jump type. 0 - next byte is jump address (-128..127), 1 - next 2 bytes are address (HI/LO, -32768..32767)
    JMP = 0x08,
    JT = 0x0A,
    JF = 0x0C,
    
    PUSHI = 0x10,   // Lower 4 bits is number from -8 to +7
    PUSHS = 0x20,   // Lower 4 bits is length from 1 to 16 (value+1)
    
    // For CALL and NEW Lower 3 bits is number of params from 0 to 6
    // A value of 7 means param count is in next byte
    CALL = 0x30, NEW = 0x38,
    
    STO = 0x50, STOMUL = 0x51, STOADD = 0x52, STOSUB = 0x53, STODIV = 0x54, STOMOD = 0x55, STOSHL = 0x56, STOSHR = 0x57,
    STOSAR = 0x58, STOAND = 0x59, STOOR = 0x5A, STOXOR = 0x5B,
    PREINC = 0x60, PREDEC = 0x61, POSTINC = 0x62, POSTDEC = 0x63, UPLUS = 0x64, UMINUS = 0x65, UNOT = 0x66, UNEG = 0x67,
    LOR = 0x70, LAND = 0x71, AND = 0x72, OR = 0x73, XOR = 0x74, EQ = 0x75, NE = 0x76, LT = 0x77,
    LE = 0x78, GT = 0x79, GE = 0x7A, SHL = 0x7B, SHR = 0x7C, SAR = 0x7D, ADD = 0x7E, SUB = 0x7F,
    MUL = 0x80, DIV = 0x81, MOD = 0x82,

    DEREF = 0x90, NEWID = 0x91, DEL = 0x92, LABEL = 0x93, END = 0x94,
};

struct Label {
    int32_t label : 20;
    uint32_t uniqueID : 12;
    int32_t fixupAddr : 20;
};

class ExecutionUnit : public Object {
public:
    ExecutionUnit(Parser* parser) : _parser(parser) { _name.set(Atom::NoAtom); }
    
    Label label();
    
    void addParam(const Atom& atom) { _params.push_back(atom); }
    
    void addCode(const char*);
    void addCode(uint32_t);
    void addCode(float);
    void addCode(const Atom&);
    void addCode(Op);
    void addCode(ExecutionUnit*);
    void addCallOrNew(bool call, uint32_t nparams);
    void addFixupJump(bool cond, Label&);
    void addJumpAndFixup(Label&);
    
    virtual String toString(uint32_t nestingLevel) const;

	void setName(const Atom& atom) { _name = atom; }
    
private:
    static uint8_t byteFromInt(uint32_t value, uint32_t index)
    {
        assert(index < 4);
        return static_cast<uint8_t>(value >> (8 * index));
    }
        
    int32_t intFromCode(uint32_t index, uint32_t size) const
    {
        uint32_t num = uintFromCode(index, size);
        uint32_t mask = 0x80 << (8 * (size - 1));
        if (num & mask) {
            return num | ~(mask - 1);
        }
        return static_cast<int32_t>(num);
    }
    
    uint32_t uintFromCode(uint32_t index, uint32_t size) const
    {
        uint32_t value = 0;
        for (int i = 0; i < size; ++i) {
            value <<= 8;
            value |= _code[index + i];
        }
        return value;
    }
    
    void addCodeInt(uint32_t value, uint32_t size)
    {
        for (int i = size - 1; i >= 0; --i) {
            _code.push_back(byteFromInt(value, i));
        }
    }
    
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

#if SHOW_CODE
    static const char* stringFromOp(Op op);
    void indentCode(String&) const;
    mutable uint32_t _nestingLevel = 0;
#endif
      
    static uint32_t _nextID;
    
    Vector<Atom> _params;
    Vector<uint8_t> _code;
    Parser* _parser;
    Vector<Object*> _objects;
    uint32_t _id = _nextID++;
	Atom _name;
};
    
}
