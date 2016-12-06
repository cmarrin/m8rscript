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

#ifdef __APPLE__
    #define ICACHE_RODATA_ATTR
    #define ICACHE_STORE_ATTR
    #define ICACHE_FLASH_ATTR
    static inline uint8_t ICACHE_FLASH_ATTR readRomByte(const uint8_t* addr) { return *addr; }
    #define ROMmemcpy memcpy
    #define ROMstrlen strlen
    #include <cstring>
    static inline char* ROMCopyString(char* dst, const char* src) { strcpy(dst, src); return dst + strlen(src); }
    #define ROMSTR(s) s
    #define debugf printf
#else
    #include "Esp.h"
#endif

namespace m8r {

static inline bool isDigit(uint8_t c)		{ return c >= '0' && c <= '9'; }
static inline bool isOctal(uint8_t c)       { return c >= '0' && c <= '7'; }
static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
static inline bool isUpper(uint8_t c)		{ return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)		{ return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)		{ return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)		{ return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)		{ return isDigit(c) || isIdFirst(c); }
static inline bool isWhitespace(uint8_t c)  { return c == ' ' || c == '\n' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }

struct Label {
    int32_t label : 20;
    uint32_t uniqueID : 12;
    int32_t matchedAddr : 20;
};

/*
    Here is the new design of Opcodes:

    Opcodes are 32 bits. There are 3 bit patterns:
    
    Opcode:6, R:8, RK:9, RK:9
    Opcode:6, R:8, N:18


    R       - Register (0..255)
    RK      - Register (0..255) or Constant (256..511)
    N       - Passed params (0..256K) or address (-128K..128K)
    L       - Local variable (0..255) - only used during initial code generation
    
    Local vs Register parameters
    ----------------------------
    During initial code generation, register indexes are relative to 0 and grow
    according to the number of register entries needed at any given time.
    The high water mark is saved in the Function when code generation for that
    Function is closed. Local variables for the function are also indexed 
    starting at 0, but in a local variable space. During initial code
    generation these are loaded into register variables by generating the LOADL
    opcode. The destination is in the register space and the source is in
    the locals space.
    
    After the Function is closed, a pass is done on the generated code. All
    register indexs are bumped by the number of locals and the LOADL opcode
    is changed to a MOVE with the destination index bumped but the source
    index left intact. So the LOADL opcode should never make it through to
    the ExecutionUnit.

    UNKNOWN     X, X, X
    RET         X, N
    END         X, X, X
    
    MOVE        R[d], RK[s], X                             ; (4)
    LOADREFK    R[d], RK[s], X
    APPENDELT   R[d], RK[s], X
 
    LOADLITA    R[d], X, X
    LOADLITO    R[d], X, X
    LOADTRUE    R[d], X, X
    LOADFALSE   R[d], X, X
    LOADNULL    R[d], X, X
 
    LOADPROP    R[d], RK[o], K[p]
    LOADELT     R[d], RK[o], RK[e]
    STOPROP     RK[o], K[p], RK[s]
    STOELT      RK[o], RK[e], RK[s]
    
    <binop> ==> LOR, LAND,                                  ; (20)
                OR, AND, XOR,
                EQ, NE, LT, LE, GT, GE,
                SHL, SHR, SAR,
                ADD, SUB, MUL, DIV, MOD
                DEREF
    
    <binop>RR   R[d], RK[s1], RK[s2]
 
    <unop> ==>  UMINUS, UNOT, UNEG                          ; (7)
                PREINC, PREDEC, POSTINC, POSTDEC
                
    <unop>R     R[d], R[s], X
 
    CALL        RK[s], N                                    ; (8)
    NEW         RK[s], N
 
    JMP         X, N
    JT          RK[s], N
    JF          RK[s], N
 
    Total: 39 instructions
*/

static constexpr uint32_t MaxRegister = 255;

enum class Op : uint8_t {
    UNKNOWN = 0x00, RET, END,
    
    MOVE = 0x10, LOADREFK, LOADLITA, LOADLITO,
    LOADPROP, LOADELT, STOPROP, STOELT, APPENDELT,
    LOADTRUE, LOADFALSE, LOADNULL,

    LOR = 0x20, LAND, OR, AND, XOR,
    EQ,  NE, LT, LE, GT, GE,
    SHL, SHR, SAR,
    ADD, SUB, MUL, DIV, MOD,
    DEREF,

    UMINUS = 0x40, UNOT, UNEG, PREINC, PREDEC, POSTINC, POSTDEC,

    CALL = 0x50, NEW, JMP, JT, JF,
    
    LAST
};

static_assert(static_cast<uint32_t>(Op::LAST) <= 0x56, "Opcode must fit in 6 bits");

static inline Op machineCodeToOp(uint32_t code) { return static_cast<Op>(code >> 26); }
static inline uint32_t machineCodeToRa(uint32_t code) { return (code >> 18) & 0xff; }
static inline uint32_t machineCodeToRb(uint32_t code) { return (code >> 9) & 0x1ff; }
static inline uint32_t machineCodeToRc(uint32_t code) { return code & 0x1ff; }
static inline uint32_t machineCodeToUN(uint32_t code) { return code & 0x3ffff; }
static inline int32_t machineCodeToSN(uint32_t code) { return static_cast<int32_t>(code << 14) >> 14; }

static inline uint32_t genMachineCodeRRR(Op op, uint32_t ra, uint32_t rb, uint32_t rc) { return (static_cast<uint32_t>(op) << 26) | ((ra & 0xff) << 18) | ((rb & 0x1ff) << 9) | (rc & 0x1ff); }
static inline uint32_t genMachineCodeRUN(Op op, uint32_t ra, uint32_t n) { return (static_cast<uint32_t>(op) << 26) | ((ra & 0xff) << 18) | (n & 0x3ffff); }
static inline uint32_t genMachineCodeRSN(Op op, uint32_t ra, int32_t n) { return (static_cast<uint32_t>(op) << 26) | ((ra & 0xff) << 18) | (n & 0x3ffff); }


////  Opcodes with params have bit patterns.
////  Upper 2 bits are 00 (values from 0x00 to 0x3f)
////  The lower 2 bits indicate the number of additional bytes:
////      00 - 1
////      01 - 2
////      10 - 4
////      11 - 8
////
////  The next 4 bits is the opcode class:
////      0000 - unused
////      0001 - PUSHID       2 byte form used
////      0010 - PUSHF        4 or 8 byte form used
////      0011 - PUSHI        1, 2, 4, 8 byte forms used
////      0100 - PUSHS        1, 2 and 4 byte forms used
////      0101 - JMP          1 and 2 byte forms used
////      0110 - JT           1 and 2 byte forms used
////      0111 - JF           1 and 2 byte forms used
////      1000 - CALL         1 byte form used
////      1001 - NEW          1 byte form used
////      1010 - PUSHO        4 byte form used
////      1011 - RET          1 byte form used
////      1100 - PUSHL        1 and 2 byte forms used - Push local variable. Param is index in _locals
////      1101 - PUSHIDREF    2 byte form used (same as PUSHID, but used for bare refs)
////      1101 - PUSHK        1 byte form used, push 1 byte ConstantId
////
//enum class Op : uint8_t {
//    UNKNOWN = 0x00,
//    PUSHID = 0x04,   // 0000 0100 - Next 2 bytes are atom
//    PUSHF  = 0x08,   // 0000 1011 - Next 4 or 8 bytes are number
//    PUSHIX = 0x0C,   // 0000 1100 - Next bytes are number
//    PUSHSX = 0x10,   // 0001 0000 - Next 1, 2, or 4 bytes is the index into the string table
//    
//    // The jump instructions use the LSB to indicate the jump type. 0 - next byte is jump address (-128..127), 1 - next 2 bytes are address (HI/LO, -32768..32767)
//    JMP = 0x14,     // 0001 0100
//    JT = 0x18,      // 0001 1000
//    JF = 0x1C,      // 0001 1100
//    
//    CALLX = 0x20,   // 0010 0000
//    NEWX = 0x24,    // 0010 0100
//    PUSHO = 0x28,   // 0010 1010
//    RETX = 0x2C,    // 0010 1100
//    PUSHLX = 0x30,  // 0011 0000
//    
//    PUSHIDREF = 0x34, // 0011 0100
//    PUSHK = 0x38,   // 0011 1000
//    
//    PUSHI = 0x40,   // Lower 4 bits is number from 0 to 15
//    CALL = 0x50,    // Lower 4 bits is number of params from 0 to 15
//    NEW = 0x60,     // Lower 4 bits is number of params from 0 to 15
//    RET = 0x70,     // Lower 4 bits is number of return values from 0 to 15
//    PUSHL = 0x80,   // Lower 4 bits is the index into _locals from 0 to 15
//
//    PUSHLITA = 0xC0, PUSHLITO = 0xC1, PUSHTRUE = 0xC2, PUSHFALSE = 0xC3, PUSHNULL = 0xC3,
//    PREINC = 0xD0, PREDEC = 0xD1, POSTINC = 0xD2, POSTDEC = 0xD3,
//    
//    // UNOP
//    UPLUS = 0xD4, UMINUS = 0xD5, UNOT = 0xD6, UNEG = 0xD7,
//    
//    DEREF = 0xD8, DEL = 0xD9, POP = 0xDA, STOPOP = 0xDB,
//    
//    // Append tos to tos-1 which must be an array
//    STOA = 0xDC,
//    
//    // tos-2 must be an object. Add value in tos to property named in tos-1
//    STOO = 0xDD,
//    
//    // Duplicate TOS
//    DUP = 0xDE,
//    
//    STO = 0xE0, 
//    
//    // Store tos in the element or property in tos-1 of the object in tos-2
//    STOELT = 0xE1, STOPROP = 0xE2,
//    
//    // BINIOP
//    LOR = 0xEC, LAND = 0xED, AND = 0xEE, OR = 0xEF,
//    XOR = 0xF0, EQ = 0xF1, NE = 0xF2, LT = 0xF3, LE = 0xF4, GT = 0xF5, GE = 0xF6, SHL = 0xF7,
//    SHR = 0xF8, SAR = 0xF9,
//    
//    // BINOP
//    ADD = 0xFA, SUB = 0xFB, MUL = 0xFC, DIV = 0xFD, MOD = 0xFE,
//    
//    END = 0xFF,
//};

#undef DEC
enum class Token : uint8_t {
    Function = 1,
    New = 2,
    Delete = 3,
    Var = 4,
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
    Do = 23,
    True = 24,
    False = 25,
    Null = 26,
    
    Bang = 33, 
    Percent = 37,
    Ampersand = 38,
    LParen = 40,
    RParen = 41,
    Star = 42,
    Plus = 43, 
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    
    Float = 48,
    Identifier = 49,
    String = 50,
    Integer = 51,
    
    Colon = 58,
    Semicolon = 59,
    LT = 60,
    STO = 61,
    GT = 62,
    Question = 63,
    
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
    
    LBracket = 91,
    RBracket = 93,
    XOR = 94,
    LBrace = 123,
    OR = 124,
    RBrace = 125,
    Twiddle = 126,
    
    Expr = 0xe0,
    PropertyAssignment = 0xe1,

    None = 0xfd,
    Error = 0xfe,
    EndOfFile = 0xff,
};

static const uint8_t MajorVersion = 0;
static const uint8_t MinorVersion = 1;

// File format is a sequence of blocks. Some blocks are simply a byte
// token indicating the block type and others are a generic format
// with the token byte followed by 2 bytes of length. Every version
// must understand Type, Version, ObjectStart and ObjectEnd. All others
// are optional can be skipped by skipping the token byte and the number
// of bytes contained in the next 2 bytes (high byte is MSB).
//
// Currently, the only properties stored are nested functions inside Function
// objects (which includes Program objects)
enum class ObjectDataType : uint8_t {
    End = 0x00,
    Type = 0x01,            // { 4 bytes: 'm', '8', 'r' }
    Version = 0x01,         // { uint8_t major, uint8_t minor }
    
    // Object
    ObjectStart = 0x10,     // Indicates start of object data
    ObjectName = 0x11,      // { uint16_t size, uint8_t name[size] }
    PropertyCount = 0x12,   // { uint16_t size = 2, uint16_t count }
    PropertyId = 0x13,      // { uint16_t size = 2, uint16_t id }
                            // Must be immediately followed by object
    ObjectEnd = 0x1f,       // Indicates end of object data

    // Program
    AtomTable = 0x20,       // { uint16_t size, uint8_t data[size] }
    StringTable = 0x21,     // { uint16_t size, uint8_t data[size] }
    ObjectCount = 0x22,     // { uint16_t size = 2, uint16_t count }
    ObjectId = 0x22,        // { uint16_t size = 2, uint16_t id }
                            // Must be immediately followed by object
    
    // Function
    Locals = 0x31,          // { uint16_t nparams, uint16_t atoms[nparams] }
    ParamEnd = 0x32,        // { uint16_t size = 2, uint16_t paramEnd }
    Code = 0x33,            // { uint16_t size, uint8_t code[size] }
};

}
