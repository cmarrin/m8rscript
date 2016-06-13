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

Value* ExecutionUnit::valueFromId(Atom id, const Object* obj) const
{
    // Start at the current object and walk up the chain
    return nullptr;
}

void ExecutionUnit::call(uint32_t nparams, Object* obj, bool isNew)
{
//    // On entry the stack has the return address, followed by nparams values, followed by the function to be called
//    size_t tos = _stack.size();
//    Value& callee = _stack[tos - nparams - 2];
//    Object* callee =
//    
//    // Fill in the local vars
//    size_t firstParam = tos - nparams - 1;
//    for (size_t i = 0; i < nparams; ++i) {
//        if (
//    }
}

void ExecutionUnit::run(Program* program)
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static void* dispatchTable[] {
        /* 0x00 */    OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x05 */ OP(PUSHID) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x0B */ OP(PUSHF)
        /* 0x0C */ OP(PUSHIX) OP(PUSHIX) OP(UNKNOWN) OP(PUSHIX)
        /* 0x10 */ OP(PUSHSX) OP(PUSHSX) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x14 */ OP(JMP) OP(JMP) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x18 */ OP(JT) OP(JT)  OP(UNKNOWN) OP(UNKNOWN)
        /* 0x1C */ OP(JF) OP(JF) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x20 */ OP(CALLX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x24 */ OP(NEWX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(PUSHO)
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
        /* 0xC0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0xD0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xD8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xE8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xFF */ OP(END)
    };
    
static_assert (sizeof(dispatchTable) == 256 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        op = static_cast<Op>(obj->codeAtIndex(i++)); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    Object* obj = program->main();
    _stack.clear();
    _stack.setFrameSize(obj->localSize());
    int i = 0;
    
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    float floatValue;
    bool boolValue;
    uint32_t size;
    Op op;
    Value leftValue, rightValue;
    
    DISPATCH;
    
    L_UNKNOWN:
        assert(0);
        return;
    L_PUSHID:
        _stack.push(Atom::atomFromRawAtom(uintFromCode(obj, i, 2)));
        i += 2;
        DISPATCH;
    L_PUSHF:
        floatValue = floatFromCode(obj, i);
        _stack.push(Value(floatValue));
        i += 4;
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            intValue = intFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            intValue = intFromCode(obj, i, size);
            i += size;
        }
        _stack.push(Value(intValue));
        DISPATCH;
    L_PUSHSX:
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        _stack.push(Value(program->stringFromId(StringId::stringIdFromRawStringId(obj->codeAtIndex(i++)))));
        DISPATCH;
    L_PUSHO:
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        if (maskOp(op, 0x0f) == Op::PUSHL) {
            intValue = intFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            assert(size <= 2);
            intValue = intFromCode(obj, i, size);
            i += size;
        }
        _stack.push(&(_stack.inFrame(intValue)));
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(obj, i, size);
        op = maskOp(op, 0x01);
        i += size;
        if (op != Op::JMP) {
            boolValue = _stack.top().toBoolValue() != 0;
            _stack.pop();
            if (op == Op::JT) {
                boolValue = !boolValue;
            }
            if (boolValue) {
                DISPATCH;
            }
        }
        i += intValue;
        DISPATCH;
    L_NEW:
    L_NEWX:
    L_CALL:
    L_CALLX:
        if (op == Op::CALLX || op == Op::NEWX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x0f);
            op = maskOp(op, 0x0f);
        }
        _stack.push(Value(i));
        call(uintValue, obj, op == Op::CALL || op == Op::CALLX);
        DISPATCH;
    L_RET:
    L_RETX:
        if (op == Op::RETX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
        DISPATCH;
    L_OPCODE:
        switch(op) {
            case Op::STOPOP:
                _stack.top(-1).setValue(_stack.top());
                _stack.pop();
                _stack.pop();
                break;
            case Op::MUL:
                rightValue = _stack.top().bakeValue();
                _stack.pop();
                leftValue = _stack.top().bakeValue();
                if (leftValue.isInteger() && rightValue.isInteger()) {
                    _stack.setTop(leftValue.asIntValue() * rightValue.asIntValue());
                } else {
                    _stack.setTop(leftValue.toFloatValue() * rightValue.toFloatValue());
                }
            default:
                break;
        }
        DISPATCH;
    L_END:
        return;
}

#if SHOW_CODE
static m8r::String toString(float value)
{
    m8r::String s;
    char buf[40];
    sprintf(buf, "%g", value);
    s = buf;
    return s;
}

static m8r::String toString(uint32_t value)
{
    m8r::String s;
    char buf[40];
    sprintf(buf, "%u", value);
    s = buf;
    return s;
}

uint32_t ExecutionUnit::findAnnotation(uint32_t addr) const
{
    for (auto annotation : annotations) {
        if (annotation.addr == addr) {
            return annotation.uniqueID;
        }
    }
    return 0;
}

void ExecutionUnit::preamble(String& s, uint32_t addr) const
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
    s += ::toString(uniqueID);
    s += "]\n";
    indentCode(s);
}
#endif

m8r::String ExecutionUnit::generateCodeString(const Program* program) const
{
#if SHOW_CODE
    m8r::String outputString;
    
	for (const auto& object : program->objects()) {
        if (object.value->hasCode()) {
            outputString += generateCodeString(program, object.value, ::toString(object.key.rawObjectId()).c_str(), _nestingLevel);
            outputString += "\n";
        }
	}
    
    outputString += generateCodeString(program, program->main(), "main", 0);
    return outputString;
#else
    return "PROGRAM\n";
#endif
}

#if SHOW_CODE
m8r::String ExecutionUnit::generateCodeString(const Program* program, const Object* obj, const char* functionName, uint32_t nestingLevel) const
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static void* dispatchTable[] {
        /* 0x00 */    OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x05 */ OP(PUSHID) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x0B */ OP(PUSHF)
        /* 0x0C */ OP(PUSHIX) OP(PUSHIX) OP(UNKNOWN) OP(PUSHIX)
        /* 0x10 */ OP(PUSHSX) OP(PUSHSX) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x14 */ OP(JMP) OP(JMP) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x18 */ OP(JT) OP(JT)  OP(UNKNOWN) OP(UNKNOWN)
        /* 0x1C */ OP(JF) OP(JF) OP(UNKNOWN) OP(UNKNOWN)

        /* 0x20 */ OP(CALLX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x24 */ OP(NEWX) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(PUSHO)
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
        /* 0xC0 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0xD0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xD8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xE8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF0 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xF8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xFF */ OP(END)
    };
    
static_assert (sizeof(dispatchTable) == 256 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        op = static_cast<Op>(obj->codeAtIndex(i++)); \
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
    else if (obj->name() && obj->name()->valid()) {
        name = program->stringFromAtom(*obj->name());
    }
    
    indentCode(outputString);
    outputString += "FUNCTION(";
    outputString += name.c_str();
    outputString += ")\n";
    
    _nestingLevel++;
    if (obj->properties()) {
        for (const auto& value : *obj->properties()) {
            if (value.value.asObjectValue() && value.value.asObjectValue()->hasCode()) {
                outputString += generateCodeString(program, value.value.asObjectValue(), "", _nestingLevel);
                outputString += "\n";
            }
        }
    }
    
    // Annotate the code to add labels
    uint32_t uniqueID = 1;
    int i = 0;
    for ( ; ; ) {
        if (i >= obj->codeSize()) {
            printf("WENT PAST THE END OF CODE\n");
            return outputString;
        }
        if (obj->codeAtIndex(i) == static_cast<uint8_t>(Op::END)) {
            break;
        }
        uint8_t code = obj->codeAtIndex(i++);
        if (code < static_cast<uint8_t>(Op::PUSHI)) {
            int count = (code & 0x03) + 1;
            int nexti = i + count;
            if (code == static_cast<uint8_t>(Op::PUSHSX)) {
                nexti += intFromCode(obj, i, count);
            }
            
            code &= 0xfc;
            Op op = static_cast<Op>(code);
            if (op == Op::JMP || op == Op::JT || op == Op::JF) {
                int32_t addr = intFromCode(obj, i, count);
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
    
    Op op;
    
    DISPATCH;
    
    L_UNKNOWN:
        outputString += "UNKNOWN\n";
        DISPATCH;
    L_PUSHID:
        preamble(outputString, i - 1);
        strValue = program->stringFromRawAtom(uintFromCode(obj, i, 2));
        i += 2;
        outputString += "ID(";
        outputString += strValue.c_str();
        outputString += ")\n";
        DISPATCH;
    L_PUSHF:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        outputString += "FLT(";
        outputString += ::toString(*(reinterpret_cast<float*>(&uintValue)));
        outputString += ")\n";
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        preamble(outputString, i - 1);
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            uintValue = uintFromCode(obj, i, size);
            i += size;
        }
        outputString += "INT(";
        outputString += ::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHSX:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        outputString += "STR(\"";
        outputString += program->stringFromId(StringId::stringIdFromRawStringId(uintValue));
        outputString += "\")\n";
        DISPATCH;
    L_PUSHO:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(obj, i, 4);
        i += 4;
        outputString += "OBJ(";
        outputString += ::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        preamble(outputString, i - 1);
        if (maskOp(op, 0x0f) == Op::PUSHL) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            uintValue = uintFromCode(obj, i, size);
            i += size;
        }
        outputString += "LOCAL(";
        outputString += program->stringFromAtom(obj->localName(uintValue));
        outputString += ")\n";
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        preamble(outputString, i - 1);
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(obj, i, size);
        op = maskOp(op, 0x01);
        outputString += (op == Op::JT) ? "JT" : ((op == Op::JF) ? "JF" : "JMP");
        outputString += " LABEL[";
        outputString += ::toString(findAnnotation(i + intValue));
        outputString += "]\n";
        i += size;
        DISPATCH;
    L_NEW:
    L_NEWX:
    L_CALL:
    L_CALLX:
        preamble(outputString, i - 1);
        if (op == Op::CALLX || op == Op::NEWX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
        
        outputString += (maskOp(op, 0x07) == Op::CALL || op == Op::CALLX) ? "CALL" : "NEW";
        outputString += "[";
        outputString += ::toString(uintValue);
        outputString += "]\n";
        DISPATCH;
    L_RET:
    L_RETX:
        preamble(outputString, i - 1);
        if (op == Op::RETX) {
            uintValue = uintFromCode(obj, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
    
        outputString += "RET[";
        outputString += ::toString(uintValue);
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
    
    OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UPLUS) OP(UMINUS) OP(UNOT) OP(UNEG)
    OP(DEREF) OP(DEL) OP(POP) OP(STOPOP)

    OP(STO) OP(STOMUL) OP(STOADD) OP(STOSUB) OP(STODIV) OP(STOMOD) OP(STOSHL) OP(STOSHR)
    OP(STOSAR) OP(STOAND) OP(STOOR) OP(STOXOR) OP(LOR) OP(LAND) OP(AND) OP(OR)
    OP(XOR) OP(EQ) OP(NE) OP(LT) OP(LE) OP(GT) OP(GE) OP(SHL)
    OP(SHR) OP(SAR) OP(ADD) OP(SUB) OP(MUL) OP(DIV) OP(MOD) OP(END)
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

void ExecutionUnit::indentCode(m8r::String& s) const
{
    for (int i = 0; i < _nestingLevel; ++i) {
        s += "    ";
    }
}

#endif

