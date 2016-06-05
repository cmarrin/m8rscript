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

namespace m8r {

enum class Op {
    PUSHID = 0x01,  // Next 2 bytes are atom
    PUSHF = 0x02,   // Next 4 bytes are number
    PUSHIX = 0x03,  // Next 4 bytes are number
    PUSHSX1 = 0x04,  // Next byte is length from 0 to 255, followed by string (includes trailing '\0')
    PUSHSX2 = 0x05,  // Next 2 bytes are length from 0 to 64435, followed by string (includes trailing '\0')

    PUSHI = 0x10,   // Lower 4 bits is number from -8 to +7
    PUSHS = 0x20,   // Lower 4 bits is length from 1 to 16 (value+1)
    CALL = 0x30,    // Lower 4 bits is number of params from 0 to 15
    
    DEREF = 0x40, NEW = 0x41, NEWID = 0x42, DEL = 0x43, 
    STO = 0x50, STOMUL = 0x51, STOADD = 0x52, STOSUB = 0x53, STODIV = 0x54, STOMOD = 0x55,
    STOSHL = 0x56, STOSHR = 0x57, STOSAR = 0x58, STOAND = 0x59, STOOR = 0x5A, STOXOR = 0x5B,
    PREINC = 0x60, PREDEC = 0x61, POSTINC = 0x62, POSTDEC = 0x63, UPLUS = 0x64, UMINUS = 0x65, UNOT = 0x66, UNEG = 0x67,
    LOR = 0x70, LAND = 0x71, AND = 0x72, OR = 0x73, XOR = 0x74, EQ = 0x75, NE = 0x70, LT = 0x70, LE = 0x70, GT = 0x70, GE = 0x70, SHL = 0x70, SHR = 0x70, SAR = 0x70, ADD = 0x70, SUB = 0x70, MUL = 0x70, DIV = 0x70, MOD = 0x70, 

    JMP = 0x80,    // Lower 6 bits is relative jump address from -64 to +63
};

struct Label {
    uint32_t label : 20;
    uint32_t uniqueID : 12;
};

class ExecutionUnit {
public:
    Label label() const
    {
        Label label;
        label.label = 0;
        label.uniqueID = _nextID++;
        return label;
    }
    
    void addParam(const Atom& atom) { _params.push_back(atom); }
    
    void addCode(const char*);
    void addCode(uint32_t);
    void addCode(float);
    void addCode(const Atom&);
    void addCode(Op);
    void addCode(Op, uint32_t);
    void addCode(ExecutionUnit*);
    
private:
    
    static uint32_t _nextID;
    
    Vector<Atom> _params;
    Vector<uint8_t> _code;
};
    
}
