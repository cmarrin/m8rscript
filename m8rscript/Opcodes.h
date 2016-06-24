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

namespace m8r {
    
struct Label {
    int32_t label : 20;
    uint32_t uniqueID : 12;
    int32_t matchedAddr : 20;
};

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
//      1100 - PUSHL - Push local variable. Param is index in _locals
//
enum class Op : uint8_t {
    UNKNOWN = 0x00,
    PUSHID = 0x05,   // 0000 0101 - Next 2 bytes are atom
    PUSHF  = 0x0B,   // 0000 1011 - Next 4 bytes are number
    PUSHIX = 0x0C,   // 0000 1100 - Next bytes are number
    PUSHSX = 0x10,   // 0001 0000
    
    // The jump instructions use the LSB to indicate the jump type. 0 - next byte is jump address (-128..127), 1 - next 2 bytes are address (HI/LO, -32768..32767)
    JMP = 0x14,     // 0001 0100
    JT = 0x18,      // 0001 1000
    JF = 0x1C,      // 0001 1100
    
    CALLX = 0x20,   // 0010 0000
    NEWX = 0x24,    // 0010 0100
    PUSHO = 0x2B,   // 0010 1011
    RETX = 0x2C,    // 0010 1100
    PUSHLX = 0x30,  // 0011 0000
    
    PUSHI = 0x40,   // Lower 4 bits is number from 0 to 15
    CALL = 0x50,    // Lower 4 bits is number of params from 0 to 15
    NEW = 0x60,     // Lower 4 bits is number of params from 0 to 15
    RET = 0x70,     // Lower 4 bits is number of return values from 0 to 15
    PUSHL = 0x80,   // Lower 4 bits is the index into _locals from 0 to 15

    PREINC = 0xD0, PREDEC = 0xD1, POSTINC = 0xD2, POSTDEC = 0xD3,
    
    // UNOP
    UPLUS = 0xD4, UMINUS = 0xD5, UNOT = 0xD6, UNEG = 0xD7,
    
    DEREF = 0xD8, DEL = 0xD9, POP = 0xDA, STOPOP = 0xDB,
    
    // Append tos to tos-1 which must be an array
    STOA = 0xDC,
    
    // tos-2 must be an object. Add value in tos to property named in tos-1
    STOO = 0xDD,
    
    STO = 0xE0, STOMUL = 0xE1, STOADD = 0xE2, STOSUB = 0xE3, STODIV = 0xE4, STOMOD = 0xE5, STOSHL = 0xE6, STOSHR = 0xE7,
    STOSAR = 0xE8, STOAND = 0xE9, STOOR = 0xEA, STOXOR = 0xEB,
    
    // BINIOP
    LOR = 0xEC, LAND = 0xED, AND = 0xEE, OR = 0xEF,
    XOR = 0xF0, EQ = 0xF1, NE = 0xF2, LT = 0xF3, LE = 0xF4, GT = 0xF5, GE = 0xF6, SHL = 0xF7,
    SHR = 0xF8, SAR = 0xF9,
    
    // BINOP
    ADD = 0xFA, SUB = 0xFB, MUL = 0xFC, DIV = 0xFD, MOD = 0xFE,
    
    END = 0xFF,
};

#undef DEC
enum class Token : uint8_t {
    Function = 1,
    New = 2,
    Delete = 3,
    Var = 4,
    Do = 10,
    While = 11,
    For = 12,
    If = 13,
    Else = 14,
    Switch = 15,
    Case = 16,
    Default = 17,
    Break = 18,
    Continue = 19,
    Return = 20,
    Unknown = 21,
    Comment = 22,
    Float = 48,
    Identifier = 49,
    String = 50,
    Integer = 51,
    
    SHRSTO = 65,
    SARSTO = 66,
    SHLSTO = 67,
    ADDSTO = 68,
    SUBSTO = 69,
    MULSTO = 70,
    DIVSTO = 71,
    MODSTO = 72,
    ANDSTO = 73,
    XORSTO = 74,
    ORSTO = 75,
    SHR = 76,
    SAR = 77,
    SHL = 78,
    INC = 79,
    DEC = 80,
    LAND = 81,
    LOR = 82,
    LE = 83,
    GE = 84,
    EQ = 85,
    NE = 86,
    
    Expr = 0xe0,
    PropertyAssignment = 0xe1,
    
    Error = 0xfe,
    EndOfFile = 0xff,
};

}
