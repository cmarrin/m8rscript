/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

// This allows umm to be turned off just for Mac. It should never be turned off for ESP
#ifdef __APPLE__
#define USE_UMM
#else
#define USE_UMM
#endif

// Do this so we can present defines for malloc/free for c files
#ifndef __cplusplus

#else

#include <array>
#include <cstdint>
#include <vector>
#include <cassert>
#include <limits>

#ifdef __APPLE__
    #define RODATA_ATTR
    #define ROMSTR_ATTR
    #define FLASH_ATTR
    #define ICACHE_FLASH_ATTR
    static inline uint8_t FLASH_ATTR readRomByte(const uint8_t* addr) { return *addr; }
    #define ROMmemcpy memcpy
    #define ROMstrlen strlen
    #define ROMstrcmp strcmp
    #define ROMstrstr strstr
    #define ROMsnprintf snprintf
    #define ROMvsnprintf vsnprintf
    #include <cstring>
static inline char* ROMCopyString(char* dst, const char* src) { strcpy(dst, src); return dst + strlen(src); }
    #define ROMSTR(s) s
    #define debugf printf
    
    //#define USE_LITTLEFS

static constexpr uint32_t HeapSize = 200000;
#else
    #include "Esp.h"
#endif

namespace m8r {

static inline bool isdigit(uint8_t c)		{ return c >= '0' && c <= '9'; }
static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
static inline bool isxdigit(uint8_t c)		{ return isHex(c) || isdigit(c); }
static inline bool isOctal(uint8_t c)       { return c >= '0' && c <= '7'; }
static inline bool isUpper(uint8_t c)		{ return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)		{ return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)		{ return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)		{ return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)		{ return isdigit(c) || isIdFirst(c); }
static inline bool isspace(uint8_t c)       { return c == ' ' || c == '\n' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }
static inline uint8_t tolower(uint8_t c)    { return isUpper(c) ? (c - 'A' + 'a') : c; }
static inline uint8_t toupper(uint8_t c)    { return isLower(c) ? (c - 'a' + 'A') : c; }

//  Class: Id/RawId template
//
//  Generic Id class

template <typename RawType>
class Id
{
public:
    class Raw
    {
        friend class Id;

    public:
        Raw() : _raw(NoId) { }
        explicit Raw(RawType raw) : _raw(raw) { }
        RawType raw() const { return _raw; }

    private:
        RawType _raw;
    };
    
    using value_type = RawType;
    
    Id() { _value = Raw(NoId); }
    explicit Id(Raw raw) { _value._raw = raw._raw; }
    Id(RawType raw) { _value._raw = raw; }
    Id(const Id& other) { _value._raw = other._value._raw; }
    Id(Id&& other) { _value._raw = other._value._raw; }

    value_type raw() const { return _value._raw; }

    const Id& operator=(const Id& other) { _value._raw = other._value._raw; return *this; }
    Id& operator=(Id& other) { _value._raw = other._value._raw; return *this; }
    const Id& operator=(const Raw& other) { _value._raw = other._raw; return *this; }
    Id& operator=(Raw& other) { _value._raw = other._raw; return *this; }
    operator bool() const { return _value._raw != NoId; }

    int operator-(const Id& other) const { return static_cast<int>(_value._raw) - static_cast<int>(other._value._raw); }
    bool operator==(const Id& other) const { return _value._raw == other._value._raw; }

private:
    static constexpr RawType NoId = std::numeric_limits<RawType>::max();

    Raw _value;
};

class StringLiteral : public Id<uint32_t> { using Id::Id; };
class ConstantId : public Id<uint8_t> { using Id::Id; };

using RawMad = uint16_t;
static constexpr RawMad NoRawMad = std::numeric_limits<RawMad>::max();

enum class MemoryType {
    Unknown,
    String,
    Character,
    Object,
    File,
    Task,
    ExecutionUnit,
    Native,
    Vector,
    UpValue,
    Network,
    Fixed, // Memory that is allocated malloc and needs storage for its size
    NumTypes
};

struct MemoryInfo{
    struct Entry
    {
        uint32_t sizeInBlocks = 0;
        uint32_t count = 0;
    };
    
    uint16_t heapSizeInBlocks = 0;
    uint16_t freeSizeInBlocks = 0;
    uint16_t blockSize = 4;
    uint16_t numAllocations = 0;
    std::array<Entry, static_cast<uint32_t>(MemoryType::NumTypes)> allocationsByType;
};

struct Label {
    int32_t label : 20;
    uint32_t uniqueID : 12;
    int32_t matchedAddr : 20;
};

/*
    Here is the new design of Opcodes:

    Opcodes are 32 bits. There are 3 bit patterns:
    
    Opcode:6, R:8, RK:9, RK:9
    Opcode:6, RK:9, N:17


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
    
    MOVE        R[d], RK[s], X
    LOADREFK    R[d], RK[s], X
    STOREFK     X, RK[d], RK[s]
    LOADUP      R[d], U[s]
    STOREUP     U[d], RK[s]
    APPENDELT   R[d], RK[s], X
 
    APPENDPROP  R[d], RK[p], RK[s]
 
    LOADLITA    R[d], X, X
    LOADLITO    R[d], X, X
    LOADTRUE    R[d], X, X
    LOADFALSE   R[d], X, X
    LOADNULL    R[d], X, X
    LOADTHIS    R[2], X, X
    
    PUSH        RK[s], X
    POP         R[d]
 
    LOADPROP    R[d], RK[o], K[p]
    LOADELT     R[d], RK[o], RK[e]
    STOPROP     R[o], K[p], RK[s]
    STOELT      R[o], RK[e], RK[s]
 
    <binop> ==> LOR, LAND,                          ; (20)
                OR, AND, XOR,
                EQ, NE, LT, LE, GT, GE,
                SHL, SHR, SAR,
                ADD, SUB, MUL, DIV, MOD
 
    <binop>RR   R[d], RK[s1], RK[s2]
 
    <unop> ==>  UMINUS, UNOT, UNEG,                 ; (7)
                PREINC, PREDEC, POSTINC, POSTDEC
                
    <unop>R     R[d], R[s], X
 
    CALL        RK[call], RK[this], NPARAMS
    NEW         RK[call], NPARAMS
    CALLPROP    RK[o], RK[p], NPARAMS
    CLOSURE     R[d], RK[s]
 
    JMP         X, N
    JT          RK[s], N
    JF          RK[s], N
    LINENO      X, N
 
    Total: 51 instructions
*/

static constexpr uint32_t MaxRegister = 255;

// Opcodes are 6 bits, 0x00 to 0x3f
enum class Op : uint8_t {
    
    MOVE = 0x00, LOADREFK, STOREFK, LOADLITA, LOADLITO,
    LOADPROP, LOADELT, STOPROP, STOELT, APPENDELT, APPENDPROP,
    LOADTRUE, LOADFALSE, LOADNULL, PUSH, POP,

    LOR = 0x10, LAND, OR, AND, XOR,
    EQ,  NE, LT, LE, GT, GE,
    SHL, SHR, SAR,
    ADD, SUB,
    MUL = 0x20, DIV, MOD,
    
    LINENO,
    
    LOADTHIS, LOADUP, STOREUP, CLOSURE, YIELD,
    
    // 0x29 - 0x2f unused (8)

    UMINUS = 0x30, UNOT, UNEG, PREINC, PREDEC, POSTINC, POSTDEC,
    CALL, NEW, CALLPROP, JMP, JT, JF,

    END = 0x3d, RET = 0x3e, UNKNOWN = 0x3f,
    
    LAST
};

static_assert(static_cast<uint32_t>(Op::LAST) <= 0x40, "Opcode must fit in 6 bits");

class Instruction {
public:
    Instruction() { _v = 0; _op = static_cast<uint32_t>(Op::END); }
    
    Instruction(Op op, uint32_t ra, uint32_t rb, uint32_t rc, bool call = false)
    {
        _op = static_cast<uint32_t>(op);
        if (call) {
            assert(ra < (1 << 9));
            _rcall = ra;
            assert(rb < (1 << 9));
            _rthis = rb;
            assert(rc < (1 << 8));
            _nparams = rc;
        } else {
            assert(ra < (1 << 8));
            _ra = ra;
            assert(rb < (1 << 9));
            _rb = rb;
            assert(rc < (1 << 9));
            _rc = rc;
        }
    }
    
    Instruction(Op op, uint32_t rn, uint32_t n)
    {
        _op = static_cast<uint32_t>(op);
        assert(rn < (1 << 9));
        _rn = rn;
        assert(n < (1 << 17));
        _un = n;
    }
    
    Instruction(Op op, uint32_t rn, int32_t n)
    {
        _op = static_cast<uint32_t>(op);
        assert(rn < (1 << 9));
        _rn = rn;
        assert(n < (1 << 17));
        _sn = n;
    }
    
    Op op() const { return static_cast<Op>(_op); }
    uint32_t ra() const { return _ra; }
    uint32_t rb() const { return _rb; }
    uint32_t rc() const { return _rc; }
    uint32_t rn() const { return _rn; }
    uint32_t un() const { return _un; }
    int32_t sn() const { return _sn; }
    uint32_t rcall() const { return _rcall; }
    uint32_t rthis() const { return _rthis; }
    uint32_t nparams() const { return _nparams; }
    
private:
    union {
        struct {
            uint32_t _op : 6;
            uint32_t _ra : 8;
            uint32_t _rb : 9;
            uint32_t _rc : 9;
        };
        struct {
            uint32_t _ : 6;
            uint32_t _rcall : 9;
            uint32_t _rthis : 9;
            uint32_t _nparams : 8;
        };
        struct {
            uint32_t __ : 6;
            uint32_t _rn : 9;
            int32_t _sn : 17;
        };
        struct {
            uint32_t ___ : 15;
            uint32_t _un : 17;
        };
        uint32_t _v;
    };
};

static_assert(sizeof(Instruction) == 4, "Instruction must be 32 bits");

#undef DEC
enum class Token : uint8_t {
    Break = 0x01,
    Case = 0x02,
    Class = 0x03,
    Constructor = 0x04,
    Continue = 0x05,
    Default = 0x06,
    Delete = 0x07,
    Do = 0x08,
    Else = 0x09,
    False = 0x0a,
    For = 0x0b,
    Function = 0x0c,
    If = 0x0d,
    New = 0x0e,
    Null = 0x0f,
    Return = 0x10,
    Switch = 0x11,
    This = 0x12,
    True = 0x13,
    Undefined = 0x14,
    Var = 0x15,
    While = 0x016,
    
    Bang = '!',
    Percent = '%',
    Ampersand = '&',
    LParen = '(',
    RParen = ')',
    Star = '*',
    Plus = '+',
    Comma = ',',
    Minus = '-',
    Period = '.',
    Slash = '/',
    
    Colon = ':',
    Semicolon = ';',
    LT = '<',
    STO = '=',
    GT = '>',
    Question = '?',
    
    LBracket = '[',
    RBracket = ']',
    XOR = '^',
    LBrace = '{',
    OR = '|',
    RBrace = '}',
    Twiddle = '~',
    
    SHRSTO = 0x80,
    SARSTO = 0x81,
    SHLSTO = 0x82,
    ADDSTO = 0x83,
    SUBSTO = 0x84,
    MULSTO = 0x85,
    DIVSTO = 0x86,
    MODSTO = 0x87,
    ANDSTO = 0x88,
    XORSTO = 0x89,
    ORSTO = 0x8a,
    SHR = 0x8b,
    SAR = 0x8c,
    SHL = 0x8d,
    INC = 0x8e,
    DEC = 0x8f,
    LAND = 0x90,
    LOR = 0x91,
    LE = 0x92,
    GE = 0x93,
    EQ = 0x94,
    NE = 0x95,
    
    Unknown = 0xc0,
    Comment = 0xc1,
    Whitespace = 0xc2,

    Float = 0xd0,
    Identifier = 0xd1,
    String = 0xd2,
    Integer = 0xd3,
    
    Expr = 0xe0,
    PropertyAssignment = 0xe1,
    Statement = 0xe2,
    DuplicateDefault = 0xe3,
    MissingVarDecl = 0xe4,
    OneVarDeclAllowed = 0xe5,
    ConstantValueRequired = 0xe6,

    None = 0xfd,
    Error = 0xfe,
    EndOfFile = 0xff,
};

static constexpr uint8_t MajorVersion = 0;
static constexpr uint8_t MinorVersion = 2;
static const char* BuildTimeStamp = __TIMESTAMP__;

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
                            // Must be immediately followed by object
    
    // Function
    Locals = 0x31,          // { uint16_t nparams, uint16_t atoms[nparams] }
    ParamEnd = 0x32,        // { uint16_t size = 2, uint16_t paramEnd }
    Code = 0x33,            // { uint16_t size, uint8_t code[size] }
};

}

#endif
