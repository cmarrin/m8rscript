/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

namespace m8rscript {

/*
    Opcodes:

    Instructions are a series of bytes. Opcode is always first
    byte and is followed by 0-3 bytes of operands. Most operands
    are 1 byte (register index, upvalue index, constant index or
    nparams). In some cases operand is a 2 byte uint16_t or
    int16_t. The are 2 consecutive bytes, MSB first.
    
    Opcodes are 6 bits. The upper 2 bits of the Opcode byte are
    "immediate" bits. They are used by some opcodes (ending in 'I')
    as an additional operand. For instance RETI uses them as the
    return count, from 0 to 3. Because of this the inline accessor
    function (e.g., opFromCode) must be used to access opcodes.
    These both ensure that the upper 2 bits are masked out and
    that if you attempt to access an opcode that should not have
    immediate bits set and it does it will assert.

    During parsing, instructions are placed in an Instruction
    object. This contains opcode, operands and an indication of
    which operands are valid. This instruction is then emitted
    in the code vector, which is a series of uint8_t.
    
    The structure of each instruction is given below, using this
    key:

    R       - Register (0..127)
    RK      - Register (0..127) or Constant (128..255)
    UN      - Line number (0..64K)
    SN      - Address (-32K..32K)
    L       - Local variable (0..127) - only used during initial code generation
    NPARAMS - Param count (0..255)
    
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

    UNKNOWN
    RET         NPARAMS
    RETI        IMM
    END
    
    MOVE        R[d], RK[s]
    LOADREFK    R[d], RK[s]
    STOREFK     RK[d], RK[s]
    LOADUP      R[d], U[s]
    APPENDELT   R[d], RK[s]
 
    APPENDPROP  R[d], RK[p], RK[s]
 
    LOADLITA    R[d]
    LOADLITO    R[d]
    LOADTRUE    R[d]
    LOADFALSE   R[d]
    LOADNULL    R[d]
    LOADTHIS    R[d]
    
    PUSH        RK[s]
    POP         R[d]
    POPX
 
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
 
    UMINUS      R[d], RK[s]
    UNOT        R[d], RK[s]
    UNEG        R[d], RK[s]

    PREINC      R[d], R[s]
    PREDEC      R[d], R[s]
    POSTINC     R[d], R[s]
    POSTDEC     R[d], R[s]

    CALL        RK[call], RK[this], NPARAMS
    NEW         RK[call], NPARAMS
    CALLPROP    RK[o], RK[p], NPARAMS
    CLOSURE     R[d], RK[s]
    
    JMP         SN
    JT          RK[s], SN
    JF          RK[s], SN
    LINENO      UN
 
    Total: 51 instructions
*/

static constexpr uint32_t MaxRegister = 127;
static constexpr uint32_t MaxParams = 256;
static constexpr int32_t MaxJump = 32767;

// Opcodes are 6 bits, 0x00 to 0x3f

// ESP RTOS defines SAR. Fix that here to avoid errors
#undef SAR

enum class Op : uint8_t {
    
    MOVE = 0x00, LOADREFK, STOREFK, LOADLITA,
    LOADLITO, LOADPROP, LOADELT, STOPROP,
    STOELT, APPENDELT, APPENDPROP, LOADTRUE,
    LOADFALSE, LOADNULL, PUSH, POP,

    LOR = 0x10, LAND, OR, AND,
    XOR, EQ,  NE, LT,
    LE, GT, GE, SHL,
    SHR, SAR, ADD, SUB,
    
    MUL = 0x20, DIV, MOD, UMINUS,
    UNOT, UNEG, PREINC, PREDEC,
    POSTINC, POSTDEC, CALL, NEW,
    CALLPROP, JMP, JT, JF,

    LINENO = 0x30, LOADTHIS, LOADUP,
    CLOSURE, YIELD, POPX, RETI, 
    
    // 0x37 - 0x3c open

    END = 0x3d, RET = 0x3e, UNKNOWN = 0x3f,
    
    LAST
};

static_assert(static_cast<uint32_t>(Op::LAST) <= 0x40, "Opcode must fit in 6 bits");

class OpInfo
{
public:
    static uint8_t size(Op op) { return array(op).size; }
    static bool aReg(Op op) { return flagFromLayout(op, Flags::a); }
    static bool bReg(Op op) { return flagFromLayout(op, Flags::b); }
    static bool cReg(Op op) { return flagFromLayout(op, Flags::c); }
    static bool dReg(Op op) { return flagFromLayout(op, Flags::d); }
    static bool imm(Op op) { return flagFromLayout(op, Flags::imm); }
    static bool params(Op op) { return flagFromLayout(op, Flags::P); }
    static bool number(Op op) { return flagFromLayout(op, Flags::N); }

private:
    // Bits here are a(0x01), b(0x02), c(0x04), sn(0x08), un(0x10)
    enum class Flags : uint8_t { None = 0, a = 0x01, b = 0x02, c = 0x04, d = 0x08, imm = 0x10, P = 0x20, N = 0x40 };
    
    // Regs a and d must be registers (<= MaxRegister)
    // Regs b and c can be reg or constant
    
    enum class Layout : uint8_t {
        None    = 0,
        A       = static_cast<uint8_t>(Flags::a),
        B       = static_cast<uint8_t>(Flags::b),
        AB      = static_cast<uint8_t>(Flags::a) | static_cast<uint8_t>(Flags::b),
        AD      = static_cast<uint8_t>(Flags::a) | static_cast<uint8_t>(Flags::d),
        BC      = static_cast<uint8_t>(Flags::b) | static_cast<uint8_t>(Flags::c),
        ABC     = static_cast<uint8_t>(Flags::a) | static_cast<uint8_t>(Flags::b) | static_cast<uint8_t>(Flags::c),
        IMM     = static_cast<uint8_t>(Flags::imm),
        P       = static_cast<uint8_t>(Flags::P),
        BP      = static_cast<uint8_t>(Flags::b) | static_cast<uint8_t>(Flags::P),
        BCP     = static_cast<uint8_t>(Flags::b) | static_cast<uint8_t>(Flags::c) | static_cast<uint8_t>(Flags::P),
        N       = static_cast<uint8_t>(Flags::N),
        AN      = static_cast<uint8_t>(Flags::a) | static_cast<uint8_t>(Flags::N),
        ABN     = static_cast<uint8_t>(Flags::a) | static_cast<uint8_t>(Flags::b) | static_cast<uint8_t>(Flags::N),
    };
    
    static bool flagFromLayout(Op op, Flags flag)
    {
        return static_cast<uint8_t>(array(op).layout) & static_cast<uint8_t>(flag);
    }
    
    struct Entry
    {
        Layout layout;
        uint8_t size;
    };

    static const Entry array(Op op)
    {
        static const Entry _array[ ] = {
/*0x00 */   { Layout::AB,   2 },   // MOVE         R[d], RK[s]
            { Layout::AB,   2 },   // LOADREFK     R[d], RK[s]
            { Layout::BC,   2 },   // STOREFK      RK[d], RK[s]
            { Layout::A,    1 },   // LOADLITA     R[d]
            { Layout::A,    1 },   // LOADLITO     R[d]
            { Layout::ABC,  3 },   // LOADPROP     R[d], RK[o], K[p]
            { Layout::ABC,  3 },   // LOADELT      R[d], RK[o], RK[e]
            { Layout::ABC,  3 },   // STOPROP      R[o], K[p], RK[s]
            { Layout::ABC,  3 },   // STOELT       R[o], RK[e], RK[s]
            { Layout::AB,   2 },   // APPENDELT    R[d], RK[s]
            { Layout::ABC,  3 },   // APPENDPROP   R[d], RK[p], RK[s]
            { Layout::A,    1 },   // LOADTRUE     R[d]
            { Layout::A,    1 },   // LOADFALSE    R[d]
            { Layout::A,    1 },   // LOADNULL     R[d]
            { Layout::B,    1 },   // PUSH         RK[s]
            { Layout::A,    1 },   // POP          R[d]
             
/*0x10 */   { Layout::ABC,  3 },   // LOR          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // LAND         R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // OR           R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // AND          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // XOR          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // EQ           R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // NE           R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // LT           R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // LE           R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // GT           R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // GE           R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // SHL          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // SHR          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // SAR          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // ADD          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // SUB          R[d], RK[s1], RK[s2]
              
/*0x20 */   { Layout::ABC,  3 },   // MUL          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // DIV          R[d], RK[s1], RK[s2]
            { Layout::ABC,  3 },   // MOD          R[d], RK[s1], RK[s2]
            
            { Layout::AB,   2 },   // UMINUS       R[d], RK[s]
            { Layout::AB,   2 },   // UNOT         R[d], RK[s]
            { Layout::AB,   2 },   // UNEG         R[d], RK[s]
            { Layout::AD,   2 },   // PREINC       R[d], R[s]
            { Layout::AD,   2 },   // PREDEC       R[d], R[s]
            { Layout::AD,   2 },   // POSTINC      R[d], R[s]
            { Layout::AD,   2 },   // POSTDEC      R[d], R[s]
 
            { Layout::BCP,  3 },   // CALL         RK[call], RK[this], NPARAMS
            { Layout::BP,   2 },   // NEW          RK[call], NPARAMS
            { Layout::BCP,  3 },   // CALLPROP     RK[o], RK[p], NPARAMS
            { Layout::N,    2 },   // JMP          SN
            { Layout::AN,   3 },   // JT           RK[s], SN
            { Layout::AN,   3 },   // JF           RK[s], SN
             
/*0x30 */   { Layout::N,    2 },   // LINENO       UN
            { Layout::A,    1 },   // LOADTHIS     R[d]
            { Layout::AD,   2 },   // LOADUP       R[d], U[s]
            { Layout::AB,   2 },   // CLOSURE      R[d], RK[s]
            { Layout::None, 0 },   // YIELD
            { Layout::None, 0 },   // POPX
            { Layout::IMM,  0 },   // RETI
            
/*0x37 */   { Layout::None, 0 },   // unused
            { Layout::None, 0 },   // unused
            { Layout::None, 0 },   // unused
            { Layout::None, 0 },   // unused
            { Layout::None, 0 },   // unused
/*0x3c */   { Layout::None, 0 },   // unused

/*0x3d */   { Layout::None, 0 },   // END
/*0x3e */   { Layout::P,    1 },   // RET          NPARAMS
/*0x3f */   { Layout::None, 0 },   // UNKNOWN
        };
        
        assert(static_cast<uint8_t>(op) < sizeof(_array) / sizeof(Entry));
        return _array[static_cast<uint8_t>(op)];
    }
};

// Opcode decomposition

static inline Op opFromByte(uint8_t c) { return static_cast<Op>(c & 0x3f); }
static inline uint8_t immFromByte(uint8_t c) { return c >> 6; }
static inline uint8_t byteFromOp(Op op) { return static_cast<uint8_t>(op); }
static inline uint8_t immFromOp(Op op) { return byteFromOp(op) >> 6; }
static inline uint8_t byteFromOp(Op op, uint8_t imm) { assert(byteFromOp(op) <= 0x3f); return byteFromOp(op) | (imm << 6); }
static inline uint8_t byteFromCode(const uint8_t*& code) { return *code++; }

static inline Op opFromCode(const uint8_t*& code)
{
    // Using this form is only for opcodes that don't
    // have any immediate bits set. Assure that here
#ifndef NDEBUG
    uint8_t c = byteFromCode(code);
    assert(immFromByte(c) == 0);
    return opFromByte(c);
#else
    return opFromByte(byteFromCode(code));
#endif
}
    
static inline Op opFromCode(const uint8_t*& code, uint8_t& imm)
{
    uint8_t c = byteFromCode(code);
    imm = immFromByte(c);
    return opFromByte(c);
}

static inline uint16_t uNFromCode(const uint8_t*& code)
{
    uint16_t n = static_cast<uint16_t>(byteFromCode(code)) << 8;
    return n | static_cast<uint16_t>(byteFromCode(code));
}
static inline int16_t sNFromCode(const uint8_t*& code) { return static_cast<int16_t>(uNFromCode(code)); }

// Constant storage definitions

enum class BuiltinConstants {
    Undefined = 0,
    Null = 1,
    Int0 = 2,
    Int1 = 3,
    AtomShort = 4,  // Next byte is Atom Id (0-255)
    AtomLong = 5,   // Next 2 bytes are Atom Id (Hi:Lo, 0-65535)
    NumBuiltins = 6
};

static inline uint8_t builtinConstantOffset() { return static_cast<uint8_t>(BuiltinConstants::NumBuiltins); }
static inline bool shortSharedAtomConstant(uint8_t reg) { return reg > MaxRegister && (reg - MaxRegister - 1) == static_cast<uint8_t>(BuiltinConstants::AtomShort); }
static inline bool longSharedAtomConstant(uint8_t reg) { return reg > MaxRegister && (reg - MaxRegister - 1) == static_cast<uint8_t>(BuiltinConstants::AtomLong); }

static inline uint8_t constantSize(uint8_t reg)
{
    if (reg <= MaxRegister) {
        return 0;
    }
    if (shortSharedAtomConstant(reg)) {
        return 1;
    }
    if (longSharedAtomConstant(reg)) {
        return 2;
    }
    return 0;
}

}
