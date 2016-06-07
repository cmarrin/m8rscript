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

#include "ExecutionUnit.h"

#include "Parser.h"
#include <cassert>

using namespace m8r;

uint32_t ExecutionUnit::_nextID = 1;

Label ExecutionUnit::label()
{
    Label label;
    label.label = _currentFunction->codeSize();
    label.uniqueID = _nextID++;
    _currentFunction->addCode(static_cast<uint8_t>(Op::LABEL));
    return label;
}
    
void ExecutionUnit::addCode(const char* s)
{
    uint32_t len = static_cast<uint32_t>(strlen(s)) + 1;
    if (len < 16) {
        _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHS) | static_cast<uint8_t>(len));
    } else if (len < 256) {
        _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHSX1));
        _currentFunction->addCode(len);
    } else {
        assert(len < 65536);
        _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHSX2));
        _currentFunction->addCode(Function::byteFromInt(len, 1));
        _currentFunction->addCode(Function::byteFromInt(len, 0));
    }
    for (const char* p = s; *p != '\0'; ++p) {
        _currentFunction->addCode(*p);
    }
    _currentFunction->addCode('\0');
}

void ExecutionUnit::addCode(uint32_t value)
{
    uint32_t size;
    Op op;
    if (value >= -127 && value <= 127) {
        size = 1;
        op = Op::PUSHIX1;
    } else if (value >= -32767 && value <= 32767) {
        size = 2;
        op = Op::PUSHIX2;
    } else {
        size = 4;
        op = Op::PUSHIX4;
    }
    _currentFunction->addCode(static_cast<uint8_t>(op));
    _currentFunction->addCodeInt(value, size);
}

void ExecutionUnit::addCode(float value)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHF));
    _currentFunction->addCodeInt(*(reinterpret_cast<uint32_t*>(&value)), 4);
}

void ExecutionUnit::addCode(const Atom& atom)
{
    _currentFunction->addCode(static_cast<uint8_t>(Op::PUSHID));
    _currentFunction->addCodeInt(atom.rawAtom(), 2);
}

void ExecutionUnit::addCode(Op value)
{
    _currentFunction->addCode(static_cast<uint8_t>(value));
}

void ExecutionUnit::addFixupJump(bool cond, Label& label)
{
    _currentFunction->addCode(static_cast<uint8_t>(cond ? Op::JT : Op::JF) | 1);
    label.fixupAddr = static_cast<int32_t>(_currentFunction->codeSize());
    _currentFunction->addCode(0);
    _currentFunction->addCode(0);
}

void ExecutionUnit::addJumpAndFixup(Label& label)
{
    int32_t jumpAddr = label.label - static_cast<int32_t>(_currentFunction->codeSize());
    if (jumpAddr >= -127 && jumpAddr <= 127) {
        _currentFunction->addCode(static_cast<uint8_t>(Op::JMP));
        _currentFunction->addCode(static_cast<uint8_t>(jumpAddr));
    } else {
        if (jumpAddr < -32767 || jumpAddr > 32767) {
            printf("JUMP ADDRESS TOO BIG TO LOOP. CODE WILL NOT WORK!\n");
            return;
        }
        _currentFunction->addCode(static_cast<uint8_t>(Op::JMP) | 0x01);
        _currentFunction->addCode(Function::byteFromInt(jumpAddr, 1));
        _currentFunction->addCode(Function::byteFromInt(jumpAddr, 0));
    }
    
    jumpAddr = static_cast<int32_t>(_currentFunction->codeSize()) - label.fixupAddr;
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printf("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n");
        return;
    }
    _currentFunction->setCodeAtIndex(label.fixupAddr, Function::byteFromInt(jumpAddr, 1));
    _currentFunction->setCodeAtIndex(label.fixupAddr + 1, Function::byteFromInt(jumpAddr, 0));
}

void ExecutionUnit::addCallOrNew(bool call, uint32_t nparams)
{
    assert(nparams < 256);
    Op op = call ? Op::CALL : Op::NEW;
    _currentFunction->addCode(static_cast<uint8_t>(op) | ((nparams > 6) ? 0x07 : nparams));
    if (nparams > 6) {
        _currentFunction->addCode(nparams);
    }
}

#if SHOW_CODE
static m8r::String toString(float value)
{
    m8r::String s;
    char buf[40];
    sprintf(buf, "%g", value);
    s.set(buf);
    return s;
}

static m8r::String toString(int32_t value)
{
    m8r::String s;
    char buf[40];
    sprintf(buf, "%d", value);
    s.set(buf);
    return s;
}

static m8r::String toString(uint32_t value)
{
    m8r::String s;
    char buf[40];
    sprintf(buf, "%u", value);
    s.set(buf);
    return s;
}

#endif

m8r::String ExecutionUnit::stringFromCode(uint32_t nestingLevel, Object* obj) const
{
#if SHOW_CODE

    #undef OP
    #define OP(op) &&L_ ## op,
    
    static void* dispatchTable[] {
        /* 0x00 */    OP(UNKNOWN)
        /* 0x01 */ OP(PUSHID)
        /* 0x02 */ OP(PUSHF)
        /* 0x03 */ OP(PUSHIX1)
        /* 0x04 */ OP(PUSHIX2)
        /* 0x05 */ OP(PUSHIX4)
        /* 0x06 */ OP(PUSHSX1)
        /* 0x07 */ OP(PUSHSX2)
        /* 0x08 */ OP(JMP) OP(JMP)
        /* 0x0A */ OP(JT) OP(JT)
        /* 0x0C */ OP(JF) OP(JF)
        /* 0x0E */     OP(UNKNOWN) OP(UNKNOWN)

        /* 0x10 */ OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x18 */      OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x20 */ OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS)
        /* 0x28 */      OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS) OP(PUSHS)

        /* 0x30 */ OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL)
        /* 0x38 */ OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW)

        /* 0x40 */     OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x48 */     OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x50 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0x58 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0x5C */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x60 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0x68 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x70 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0x78 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0x80 */ OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0x83 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x88 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x90 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(LABEL) OP(END)
        /* 0x94 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x98 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xD0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xD8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xF0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xF8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
    };

    #undef DISPATCH
    #define DISPATCH { \
        op = static_cast<Op>(obj->codeAtIndex(i++)); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    m8r::String outputString;

	for (int i = 0; i < obj->numObjects(); ++i) {
		outputString += stringFromCode(nestingLevel + 1, obj->objectAtIndex(i));
        outputString += "\n";
	}
    
    _nestingLevel = nestingLevel;
	
	m8r::String name = "<anonymous>";
    if (obj->name() && obj->name()->valid()) {
        _parser->stringFromAtom(name, *obj->name());
    }
    
    indentCode(outputString);
    outputString += "FUNCTION(";
    outputString += name.c_str();
    outputString += ")\n";
    
    _nestingLevel++;
	
    int i = 0;
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    uint32_t size;
    
    Op op;
    
    DISPATCH;
    
    L_UNKNOWN:
        outputString += "UNKNOWN\n";
        DISPATCH;
    L_PUSHID:
        _parser->stringFromRawAtom(strValue, uintFromCode(obj, i, 2));
        i += 2;
        indentCode(outputString);
        outputString += "ID(";
        outputString += strValue.c_str();
        outputString += ")\n";
        DISPATCH;
    L_PUSHF:
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        indentCode(outputString);
        outputString += "FLT(";
        outputString += ::toString(*(reinterpret_cast<float*>(&uintValue)));
        outputString += ")\n";
        DISPATCH;
    L_PUSHI:
    L_PUSHIX1:
    L_PUSHIX2:
    L_PUSHIX4:
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            intValue = intFromOp(op, 0x0f);
        } else {
            size = (op == Op::PUSHIX1) ? 1 : ((op == Op::PUSHIX2) ? 2 : 4);
            intValue = intFromCode(obj, i, size);
            i += size;
        }
        indentCode(outputString);
        outputString += "INT(";
        outputString += ::toString(intValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHS:
    L_PUSHSX1:
    L_PUSHSX2:
        if (maskOp(op, 0x0f) == Op::PUSHS) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = (op == Op::PUSHSX1) ? 1 : 2;
            uintValue = uintFromCode(obj, i, size);
            i += size;
        }
        indentCode(outputString);
        outputString += "STR(\"";
        outputString += obj->stringFromCode(i, uintValue);
        outputString += ")\n";
        i += uintValue;
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(obj, i, size);
        op = maskOp(op, 0x01);
        indentCode(outputString);
        outputString += (op == Op::JT) ? "JT" : ((op == Op::JF) ? "JF" : "JMP");
        outputString += "[";
        outputString += ::toString(intValue);
        outputString += "]\n";
        i += size;
        if (op == Op::JMP) {
            --_nestingLevel;
        }
        DISPATCH;
    L_NEW:
    L_CALL:
        uintValue = uintFromOp(op, 0x07);
        if (uintValue == 0x07) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        }
        indentCode(outputString);
        outputString += (maskOp(op, 0x07) == Op::CALL) ? "CALL" : "NEW";
        outputString += "[";
        outputString += ::toString(uintValue);
        outputString += "]\n";
        DISPATCH;
    L_OPCODE:
        indentCode(outputString);
        outputString += "OP(";
        outputString += stringFromOp(op);
        outputString += ")\n";
        DISPATCH;
    L_LABEL:
        indentCode(outputString);
        outputString += "LABEL\n";
        ++_nestingLevel;
        DISPATCH;
    L_END:
        _nestingLevel--;
        indentCode(outputString);
        outputString += "END\n";
        return outputString;
    
    
    
    
//#if SHOW_CODE
//    printf("\n");
//    indentCode();
//    printf("LABEL[%d]\n", lbl.uniqueID);
//    _nestingLevel++;
//#endif
//
//#if SHOW_CODE
//    _nestingLevel--;
//    indentCode();
//    printf("END\n");
//    printf("\n");
//#endif

#endif
}

#if SHOW_CODE
struct CodeMap
{
    Op op;
    const char* s;
};

#undef OP
#define OP(op) { Op::op, #op },

static CodeMap opcodes[] = {
    OP(PUSHID)
    OP(PUSHF)
    OP(PUSHIX1)
    OP(PUSHIX2)
    OP(PUSHIX4)
    OP(PUSHSX1)
    OP(PUSHSX2)
    
    OP(JMP)
    OP(JT)
    OP(JF)
    
    OP(PUSHI)
    OP(PUSHS)
    
    OP(CALL) OP(NEW)
    
    OP(STO) OP(STOMUL) OP(STOADD) OP(STOSUB) OP(STODIV) OP(STOMOD)
    OP(STOSHL) OP(STOSHR) OP(STOSAR) OP(STOAND) OP(STOOR) OP(STOXOR)
    OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UPLUS) OP(UMINUS) OP(UNOT) OP(UNEG)
    OP(LOR) OP(LAND) OP(AND) OP(OR) OP(XOR) OP(EQ) OP(NE) OP(LT) OP(LE) OP(GT) OP(GE)
    OP(SHL) OP(SHR) OP(SAR) OP(ADD) OP(SUB) OP(MUL) OP(DIV) OP(MOD)

    OP(DEREF) OP(NEWID) OP(DEL) OP(LABEL) OP(END)
};

const char* ExecutionUnit::stringFromOp(Op op)
{
    for (auto c : opcodes) {
        if (c.op == op) {
            return c.s;
        }
    }
    return "UNKNOWN";
}

void ExecutionUnit::indentCode(String& s) const
{
    for (int i = 0; i < _nestingLevel; ++i) {
        s += "    ";
    }
}

#endif

