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

#include "CodePrinter.h"

#include "Float.h"
#include "Parser.h"
#include "SystemInterface.h"

using namespace m8r;

bool CodePrinter::printError(const char* s) const
{
    ++_nerrors;
    if (_system) {
        _system->printf("Runtime error: %s\n", s);
    }
    if (_nerrors > 10) {
        if (_system) {
            _system->printf("\n\nToo many runtime errors, exiting...\n");
        }
        return false;
    }
    return true;
}

uint32_t CodePrinter::findAnnotation(uint32_t addr) const
{
    for (auto annotation : annotations) {
        if (annotation.addr == addr) {
            return annotation.uniqueID;
        }
    }
    return 0;
}

void CodePrinter::preamble(String& s, uint32_t addr) const
{
    uint32_t uniqueID = findAnnotation(addr);
    if (!uniqueID) {
        indentCode(s);
        return;
    }
    if (_nestingLevel) {
        --_nestingLevel;
        indentCode(s);
        ++_nestingLevel;
    }
    s += "LABEL[";
    s += Value::toString(uniqueID);
    s += "]\n";
    indentCode(s);
}

m8r::String CodePrinter::generateCodeString(const Program* program) const
{
    m8r::String outputString;
    
	for (const auto& object : program->objects()) {
        if (object.value->code()) {
            uint32_t rawId = object.key.raw();
            outputString += generateCodeString(program, object.value, Value::toString(rawId).c_str(), _nestingLevel);
            outputString += "\n";
        }
	}
    
    outputString += generateCodeString(program, program, "main", 0);
    return outputString;
}

m8r::String CodePrinter::generateCodeString(const Program* program, const Object* obj, const char* functionName, uint32_t nestingLevel) const
{
    #undef OP
    #define OP(op) &&L_ ## op,
    static const void* dispatchTable[] {
        /* 0x00 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x04 */ OP(UNKNOWN) OP(PUSHID) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x08 */ OP(UNKNOWN) OP(UNKNOWN) OP(PUSHF) OP(PUSHF)
        /* 0x0C */ OP(PUSHIX) OP(PUSHIX) OP(PUSHIX) OP(PUSHIX)
        /* 0x10 */ OP(PUSHSX) OP(PUSHSX) OP(PUSHSX) OP(UNKNOWN)
        /* 0x14 */ OP(JMP) OP(JMP) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x18 */ OP(JT) OP(JT)  OP(UNKNOWN) OP(UNKNOWN)
        /* 0x1C */ OP(JF) OP(JF) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x20 */ OP(CALLX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x24 */ OP(NEWX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN) OP(UNKNOWN) OP(PUSHO) OP(UNKNOWN)
        /* 0x2C */ OP(RETX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x30 */ OP(PUSHLX)  OP(PUSHLX)  OP(UNKNOWN)  OP(UNKNOWN)
        
        /* 0x34 */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x38 */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x3c */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x40 */ OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x48 */      OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI) OP(PUSHI)
        /* 0x50 */ OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL)
        /* 0x58 */      OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL) OP(CALL)
        /* 0x60 */ OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW)
        /* 0x68 */      OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW) OP(NEW)
        /* 0x70 */ OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET)
        /* 0x78 */      OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET) OP(RET)
        /* 0x80 */ OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL)
        /* 0x88 */      OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL) OP(PUSHL)

        /* 0x90 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x98 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xA8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xB8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC0 */      OP(OPCODE) OP(OPCODE) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0xD0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xD8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xE8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xFF */ OP(END)
    };
    
static_assert (sizeof(dispatchTable) == 256 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        op = static_cast<Op>(code[i++]); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    m8r::String outputString;

    _nestingLevel = nestingLevel;
	
	m8r::String name;
    if (functionName[0] != '\0') {
        name = "<";
        name += functionName;
        name += ">";
    }
    else {
        name = "anonymous>";
    }
    
    indentCode(outputString);
    outputString += "FUNCTION(";
    outputString += name.c_str();
    outputString += ")\n";
    
    _nestingLevel++;
    for (uint32_t i = 0; i < obj->propertyCount(); ++i) {
        const Value& value = obj->property(i);
        if (value.isNone()) {
            continue;
        }
        if (value.asObjectValue() && value.asObjectValue()->code()) {
            Atom name = obj->propertyName(i);
            outputString += generateCodeString(program, value.asObjectValue(), program->stringFromAtom(name).c_str(), _nestingLevel);
            outputString += "\n";
        }
    }
    
    const uint8_t* code = &(obj->code()->at(0));

    // Annotate the code to add labels
    uint32_t uniqueID = 1;
    uint32_t i = 0;
    for ( ; ; ) {
        if (i >= obj->code()->size()) {
            outputString += "\n\nWENT PAST THE END OF CODE\n\n";
            return outputString;
        }
        if (obj->code()->at(i) == static_cast<uint8_t>(Op::END)) {
            break;
        }
        uint8_t c = obj->code()->at(i++);
        Op op = maskOp(static_cast<Op>(c), 0x03);
        
        if (op < Op::PUSHI) {
            uint32_t count = sizeFromOp(op);
            int nexti = i + count;
            
            c &= 0xfc;
            if (op == Op::JMP || op == Op::JT || op == Op::JF) {
                int32_t addr = intFromCode(code, i, count);
                Annotation annotation = { static_cast<uint32_t>(i + addr), uniqueID++ };
                annotations.push_back(annotation);
            }
            i = nexti;
        }
    }
    
    i = 0;
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    uint32_t size;
    Atom localName;
    
    Op op;
    
    DISPATCH;
    
    L_UNKNOWN:
        outputString += "UNKNOWN\n";
        DISPATCH;
    L_PUSHID:
        preamble(outputString, i - 1);
        strValue = program->stringFromAtom(Atom(uintFromCode(code, i, 2)));
        i += 2;
        outputString += "ID(";
        outputString += strValue.c_str();
        outputString += ")\n";
        DISPATCH;
    L_PUSHF:
        preamble(outputString, i - 1);
        size = sizeFromOp(op);
        uintValue = uintFromCode(code, i, size);
        i += size;
        outputString += "FLT(";
        outputString += Value::toString(Float::make(uintValue));
        outputString += ")\n";
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        preamble(outputString, i - 1);
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = sizeFromOp(op);
            uintValue = uintFromCode(code, i, size);
            i += size;
        }
        outputString += "INT(";
        outputString += Value::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHSX:
        preamble(outputString, i - 1);
        size = sizeFromOp(op);
        uintValue = uintFromCode(code, i, size);
        i += size;
        outputString += "STR(\"";
        outputString += program->stringFromStringLiteral(StringLiteral(uintValue));
        outputString += "\")\n";
        DISPATCH;
    L_PUSHO:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(code, i, 4);
        i += 4;
        outputString += "OBJ(";
        outputString += Value::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        preamble(outputString, i - 1);
        if (maskOp(op, 0x0f) == Op::PUSHL) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = sizeFromOp(op);
            uintValue = uintFromCode(code, i, size);
            i += size;
        }
        outputString += "LOCAL(";
        localName = obj->localName(uintValue);
        outputString += localName ? program->stringFromAtom(localName) : "<UNKNOWN>";
        outputString += ")\n";
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        preamble(outputString, i - 1);
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(code, i, size);
        op = maskOp(op, 0x01);
        outputString += (op == Op::JT) ? "JT" : ((op == Op::JF) ? "JF" : "JMP");
        outputString += " LABEL[";
        outputString += Value::toString(findAnnotation(i + intValue));
        outputString += "]\n";
        i += size;
        DISPATCH;
    L_NEW:
    L_NEWX:
    L_CALL:
    L_CALLX:
        preamble(outputString, i - 1);
        if (op == Op::CALLX || op == Op::NEWX) {
            uintValue = uintFromCode(code, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
        
        outputString += (maskOp(op, 0x07) == Op::CALL || op == Op::CALLX) ? "CALL" : "NEW";
        outputString += "[";
        outputString += Value::toString(uintValue);
        outputString += "]\n";
        DISPATCH;
    L_RET:
    L_RETX:
        preamble(outputString, i - 1);
        if (op == Op::RETX) {
            uintValue = uintFromCode(code, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
    
        outputString += "RET[";
        outputString += Value::toString(uintValue);
        outputString += "]\n";
        DISPATCH;
    L_OPCODE:
        preamble(outputString, i - 1);
        outputString += "OP(";
        outputString += stringFromOp(op);
        outputString += ")\n";
        DISPATCH;
    L_END:
        _nestingLevel--;
        preamble(outputString, i - 1);
        outputString += "END\n";
        return outputString;
}

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
    OP(PUSHIX)
    OP(PUSHSX)
    
    OP(JMP)
    OP(JT)
    OP(JF)
    
    OP(CALLX)
    OP(NEWX)
    OP(PUSHO)
    OP(RETX)
    OP(PUSHLX)

    OP(PUSHI)
    OP(CALL)
    OP(NEW)
    OP(RET)
    OP(PUSHL)
    
    OP(PUSHLITA) OP(PUSHLITO)
    
    OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UPLUS) OP(UMINUS) OP(UNOT) OP(UNEG)
    OP(DEREF) OP(DEL) OP(POP) OP(STOPOP)
    
    OP(STOA)
    OP(STOO)

    OP(STO) OP(STOMUL) OP(STOADD) OP(STOSUB) OP(STODIV) OP(STOMOD) OP(STOSHL) OP(STOSHR)
    OP(STOSAR) OP(STOAND) OP(STOOR) OP(STOXOR) OP(LOR) OP(LAND) OP(AND) OP(OR)
    OP(XOR) OP(EQ) OP(NE) OP(LT) OP(LE) OP(GT) OP(GE) OP(SHL)
    OP(SHR) OP(SAR) OP(ADD) OP(SUB) OP(MUL) OP(DIV) OP(MOD) OP(END)
};

const char* CodePrinter::stringFromOp(Op op)
{
    for (auto c : opcodes) {
        if (c.op == op) {
            return c.s;
        }
    }
    return "UNKNOWN";
}

void CodePrinter::indentCode(m8r::String& s) const
{
    for (uint32_t i = 0; i < _nestingLevel; ++i) {
        s += "    ";
    }
}

