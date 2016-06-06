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

#include "Scanner.h"
#include <cassert>
#include <cstdio>

using namespace m8r;

uint32_t ExecutionUnit::_nextID = 1;

void ExecutionUnit::addCode(const char* s)
{
    uint32_t len = static_cast<uint32_t>(strlen(s)) + 1;
    if (len < 16) {
        _code.push_back(static_cast<uint8_t>(Op::PUSHS) | static_cast<uint8_t>(len));
    } else if (len < 256) {
        _code.push_back(static_cast<uint8_t>(Op::PUSHSX1));
        _code.push_back(len);
    } else {
        assert(len < 65536);
        _code.push_back(static_cast<uint8_t>(Op::PUSHSX2));
        _code.push_back(byteFromInt(len, 1));
        _code.push_back(byteFromInt(len, 0));
    }
    for (const char* p = s; *p != '\0'; ++p) {
        _code.push_back(*p);
    }
    _code.push_back('\0');
}

void ExecutionUnit::addCode(uint32_t)
{
}

void ExecutionUnit::addCode(float)
{
}

void ExecutionUnit::addCode(const Atom& atom)
{
    _code.push_back(static_cast<uint8_t>(Op::PUSHID));
    addCodeInt(atom.rawAtom(), 2);
}

void ExecutionUnit::addCode(Op value)
{
}

void ExecutionUnit::addFixupJump(bool cond, Label& label)
{
    _code.push_back(static_cast<uint8_t>(cond ? Op::JT : Op::JF) | 1);
    label.fixupAddr = static_cast<int32_t>(_code.size());
    _code.push_back(0);
    _code.push_back(0);
}

void ExecutionUnit::addJumpAndFixup(Label& label)
{
    int32_t jumpAddr = label.label - static_cast<int32_t>(_code.size());
    if (jumpAddr >= -127 && jumpAddr <= 127) {
        _code.push_back(static_cast<uint8_t>(Op::JMP));
        _code.push_back(static_cast<uint8_t>(jumpAddr));
    } else {
        if (jumpAddr < -32767 || jumpAddr > 32767) {
            printf("JUMP ADDRESS TOO BIG TO LOOP. CODE WILL NOT WORK!\n");
            return;
        }
        _code.push_back(static_cast<uint8_t>(Op::JMP) | 0x01);
        _code.push_back(byteFromInt(jumpAddr, 1));
        _code.push_back(byteFromInt(jumpAddr, 0));
    }
    
    jumpAddr = static_cast<int32_t>(_code.size()) - label.fixupAddr;
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printf("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n");
        return;
    }
    _code[label.fixupAddr] = byteFromInt(jumpAddr, 1);
    _code[label.fixupAddr + 1] = byteFromInt(jumpAddr, 0);
}

void ExecutionUnit::addCode(Op, uint32_t)
{
}

void ExecutionUnit::addCode(ExecutionUnit*)
{
}

void ExecutionUnit::printCode() const
{
#if SHOW_CODE
    int i = 0;
    while (i < _code.size()) {
        indentCode();
        
        switch(static_cast<Op>(_code[i++])) {
            case Op::PUSHID: {
                String s;
                _scanner->stringFromRawAtom(s, intFromCode(i, 2));
                printf("ID(%s)\n", s.c_str());
                break;
            }
            default:
                break;
        }
    }
    
    
    
    
//#if SHOW_CODE
//    indentCode();
//    printf("ID(%s)\n", _atomTable.toString(value).c_str());
//#endif
//
//#if SHOW_CODE
//    indentCode();
//    printf("INT(%d)\n", value);
//#endif
//
//#if SHOW_CODE
//    indentCode();
//    printf("FLT(%g)\n", value);
//#endif
//
//#if SHOW_CODE
//    indentCode();
//    printf("OP(%s)\n", stringFromOp(value));
//#endif
//
//#if SHOW_CODE
//    indentCode();
//    printf("EU(%p)\n", value);
//#endif
//
//#if SHOW_CODE
//    indentCode();
//    printf("OP(%s[%d])\n", stringFromOp(value), param);
//#endif
//
//#if SHOW_CODE
//    printf("\n");
//    indentCode();
//    printf("LABEL[%d]\n", lbl.uniqueID);
//    _nestingLevel++;
//#endif
//
//#if SHOW_CODE
//    indentCode();
//    printf("EU(%p)\n", value);
//#endif
//
//#if SHOW_CODE
//    indentCode();
//    printf("OP(%s[%d])\n", stringFromOp(value), param);
//#endif
//
//#if SHOW_CODE
//    printf("\n");
//    indentCode();
//    printf("FUNCTION\n");
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

#define OP(op) { Op::op, #op },

static CodeMap opcodes[] = {
    OP(PUSHID)
    OP(PUSHF)
    OP(PUSHIX)
    OP(PUSHSX1)
    OP(PUSHSX2)
    OP(JMP)
    OP(JT)
    OP(JF)
    
    OP(PUSHI)
    OP(PUSHS)
    OP(CALL)
    
    OP(DEREF) OP(NEW) OP(NEWID) OP(DEL)
    OP(STO) OP(STOMUL) OP(STOADD) OP(STOSUB) OP(STODIV) OP(STOMOD)
    OP(STOSHL) OP(STOSHR) OP(STOSAR) OP(STOAND) OP(STOOR) OP(STOXOR)
    OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UPLUS) OP(UMINUS) OP(UNOT) OP(UNEG)
    OP(LOR) OP(LAND) OP(AND) OP(OR) OP(XOR) OP(EQ) OP(NE) OP(LT) OP(LE) OP(GT) OP(GE)
    OP(SHL) OP(SHR) OP(SAR) OP(ADD) OP(SUB) OP(MUL) OP(DIV) OP(MOD)
};

const char* stringFromOp(Op op)
{
    for (auto c : opcodes) {
        if (c.op == op) {
            return c.s;
        }
    }
    return "UNKNOWN";
}

void ExecutionUnit::indentCode() const
{
    for (int i = 0; i < _nestingLevel; ++i) {
        printf("    ");
    }
}

#endif

