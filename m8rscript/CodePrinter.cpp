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

#include "ExecutionUnit.h"
#include "Float.h"
#include "SystemInterface.h"

using namespace m8r;

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
    
	for (uint16_t i = 0; ; i++) {
        Object* obj = program->obj(ObjectId(i));
        if (!obj || obj == program) {
            break;
        }
        if (obj->code()) {
            outputString += generateCodeString(program, obj, Value::toString(i).c_str(), _nestingLevel);
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
        /* 0x04 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x08 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x0C */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x10 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x14 */ OP(JMP) OP(JMP) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x18 */ OP(JT) OP(JT)  OP(UNKNOWN) OP(UNKNOWN)
        /* 0x1C */ OP(JF) OP(JF) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x20 */ OP(CALLX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x24 */ OP(NEWX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x2C */ OP(RETX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x30 */ OP(PUSHLX)  OP(PUSHLX)  OP(UNKNOWN)  OP(UNKNOWN)
        
        /* 0x34 */      OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x38 */      OP(PUSHK)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x3c */      OP(PUSHREFK)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x40 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x48 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
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
        /* 0xD8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(UNKNOWN)
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
        Object* object = program->obj(value);
        if (object && object->code()) {
            Atom name = obj->propertyName(i);
            outputString += generateCodeString(program, object, program->stringFromAtom(name).c_str(), _nestingLevel);
            outputString += "\n";
        }
    }
    
    const uint32_t* code = &(obj->code()->at(0));

    // Annotate the code to add labels
    uint32_t uniqueID = 1;
    uint32_t i = 0;
    for (uint32_t i = 0; ; ++i) {
        if (i >= obj->code()->size()) {
            outputString += "\n\nWENT PAST THE END OF CODE\n\n";
            return outputString;
        }

        uint32_t c = obj->code()->at(i++);
        Op op = machineCodeToOp(c);
        if (op == Op::END) {
            break;
        }

        if (op == Op::JT || op == Op::JF || op == Op::JMP) {
            int16_t addr = machineCodeToSN(c);
            Annotation annotation = { static_cast<uint32_t>(i + addr), uniqueID++ };
            annotations.push_back(annotation);
        }
    }
    
    // Display the constants
    if (obj->isFunction()) {
        const Function* function = static_cast<const Function*>(obj);
        
        // We don't show the first Constant, it is a dummy error value
        if (function->constantCount() > 1) {
            indentCode(outputString);
            outputString += "CONSTANTS:\n";
            _nestingLevel++;

            for (uint8_t i = 1; i < function->constantCount(); ++i) {
                Value constant = function->constant(ConstantId(i));
                indentCode(outputString);
                outputString += "[" + Value::toString(i) + "] = ";
                showValue(program, outputString, constant);
                outputString += "\n";
            }
            _nestingLevel--;
            outputString += "\n";
        }
    }

    indentCode(outputString);
    outputString += "CODE:\n";
    _nestingLevel++;

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
    L_PUSHREFK:
    L_PUSHK:
        preamble(outputString, i - 1);
        uintValue = ExecutionUnit::uintFromCode(code, i, 1);
        i += 1;
        outputString += (op == Op::PUSHK) ? "CONST(" : "CONSTREF)";
        outputString += Value::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        preamble(outputString, i - 1);
        if (ExecutionUnit::maskOp(op, 0x0f) == Op::PUSHL) {
            uintValue = ExecutionUnit::uintFromOp(op, 0x0f);
        } else {
            size = ExecutionUnit::sizeFromOp(op);
            uintValue = ExecutionUnit::uintFromCode(code, i, size);
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
        size = ExecutionUnit::intFromOp(op, 0x01) + 1;
        intValue = ExecutionUnit::intFromCode(code, i, size);
        op = ExecutionUnit::maskOp(op, 0x01);
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
            uintValue = ExecutionUnit::uintFromCode(code, i, 1);
            i += 1;
        } else {
            uintValue = ExecutionUnit::uintFromOp(op, 0x07);
        }
        
        outputString += (ExecutionUnit::maskOp(op, 0x07) == Op::CALL || op == Op::CALLX) ? "CALL" : "NEW";
        outputString += "[";
        outputString += Value::toString(uintValue);
        outputString += "]\n";
        DISPATCH;
    L_RET:
    L_RETX:
        preamble(outputString, i - 1);
        if (op == Op::RETX) {
            uintValue = ExecutionUnit::uintFromCode(code, i, 1);
            i += 1;
        } else {
            uintValue = ExecutionUnit::uintFromOp(op, 0x07);
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
        _nestingLevel--;
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
    OP(JMP)
    OP(JT)
    OP(JF)
    
    OP(CALLX)
    OP(NEWX)
    OP(RETX)
    OP(PUSHLX)

    OP(CALL)
    OP(NEW)
    OP(RET)
    OP(PUSHL)
    
    OP(PUSHLITA) OP(PUSHLITO) OP(PUSHTRUE) OP(PUSHFALSE) OP(PUSHNULL)
    
    OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UPLUS) OP(UMINUS) OP(UNOT) OP(UNEG)
    OP(DEREF) OP(DEL) OP(POP) OP(STOPOP)
    
    OP(STOA)
    OP(STOO)
    OP(DUP)

    OP(STO) OP(STOELT) OP(STOPROP)
    
    OP(LOR) OP(LAND) OP(AND) OP(OR) OP(XOR) OP(EQ) OP(NE) OP(LT) OP(LE) OP(GT) OP(GE) OP(SHL)
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

void CodePrinter::showValue(const Program* program, m8r::String& s, const Value& value) const
{
    switch(value.type()) {
        case Value::Type::None: s += "NONE"; break;
        case Value::Type::Float: s += "FLT(" + Value::toString(value.asFloatValue()) + ")"; break;
        case Value::Type::Integer: s += "INT(" + Value::toString(value.asIntValue()) + ")"; break;
        case Value::Type::String: s += "***String***"; break;
        case Value::Type::StringLiteral: s += "STR(\"" + String(program->stringFromStringLiteral(value.asStringLiteralValue())) + "\")"; break;
        case Value::Type::Id: s += "ATOM(\"" + program->stringFromAtom(value.asIdValue()) + "\")"; break;
        case Value::Type::ElementRef: s += "***ElementRef***"; break;
        case Value::Type::PropertyRef: s += "***PropertyRef***"; break;
        case Value::Type::PreviousFrame: s += "***PreviousFrame***"; break;
        case Value::Type::PreviousPC: s += "***PreviousPC***"; break;
        case Value::Type::PreviousObject: s += "***PreviousObject***"; break;
        case Value::Type::Object: {
            ObjectId objectId = value.asObjectIdValue();
            Object* obj = program->obj(objectId);
            if (obj->isFunction()) {
                _nestingLevel++;
                s += "\n";
                s += generateCodeString(program, obj, Value::toString(objectId.raw()).c_str(), _nestingLevel);
                _nestingLevel--;
                break;
            }
            s += "OBJ(" + Value::toString(value.asObjectIdValue().raw()) + ")";
            break;
        }
    }
}
