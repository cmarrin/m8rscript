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
#include <cmath>
#include <string>

using namespace m8r;

bool ExecutionUnit::printError(const char* s, void (*printer)(const char*)) const
{
    ++_nerrors;
    printer("Runtime error: ");
    printer(s);
    printer("\n");
    if (++_nerrors > 10) {
        printer("\n\nToo many runtime errors, exiting...\n");
        return false;
    }
    return true;
}

Value* ExecutionUnit::valueFromId(Atom id, const Object* obj) const
{
    // Start at the current object and walk up the chain
    return nullptr;
}

uint32_t ExecutionUnit::call(Program* program, uint32_t nparams, Object* obj, bool isNew)
{
// On entry the stack has:
//      tos           ==> nparams values
//      tos-nparams   ==> the function to be called
//
//    size_t tos = _stack.size();
//    Value& callee = _stack[tos - nparams - 2];
//    Object* callee =
//    
//    // Fill in the local vars
//    size_t firstParam = tos - nparams - 1;
//    for (size_t i = 0; i < nparams; ++i) {
//        if (
//    }

    Value callee = _stack.top(-nparams);
    if (callee.type() == Value::Type::Id) {
        callee = deref(program, program->global(), callee);
    }
    
    // FIXME: when error occurs (return < 0) log an error or something
    int32_t returnCount = callee.call(_stack, nparams);
    if (returnCount < 0) {
        returnCount = 0;
    }
    return returnCount;
//
// On return the return address has:
//      tos       ==> count returned values
//      tos-count ==> nparams values
//      tos-count-nparams ==> the function to be called
//
}

Value ExecutionUnit::deref(Program* program, Object* obj, const Value& derefValue)
{
    if (derefValue.isInteger()) {
        return Value();
    }
    int32_t index = obj->propertyIndex(propertyNameFromValue(program, derefValue), true);
    if (index < 0) {
        return Value();
    }
    return obj->propertyRef(index);
}

bool ExecutionUnit::deref(Program* program, Value& objectValue, const Value& derefValue)
{
    if (objectValue.type() == Value::Type::Id) {
        objectValue = deref(program, program->global(), objectValue);
        return deref(program, objectValue, derefValue);
    }
    
    if (objectValue.type() == Value::Type::PropertyRef) {
        objectValue = objectValue.appendPropertyRef(derefValue);
        return !objectValue.isNone();
    }
    
    Object* obj = objectValue.toObjectValue();
    if (!obj) {
        return false;
    }
    if (derefValue.isInteger()) {
        objectValue = obj->elementRef(derefValue.toIntValue());
        return !objectValue.isNone();
    }
    int32_t index = obj->propertyIndex(propertyNameFromValue(program, derefValue), true);
    if (index < 0) {
        return false;
    }
    objectValue = obj->propertyRef(index);
    return true;
}

Atom ExecutionUnit::propertyNameFromValue(Program* program, const Value& value)
{
    if (value.canBeBaked()) {
        return propertyNameFromValue(program, value.bakeValue());
    }
    switch(value.type()) {
        case Value::Type::String: return program->atomizeString(value.asStringValue());
        case Value::Type::Id: return value.asIdValue();
        case Value::Type::Integer: {
            char buf[40];
            sprintf(buf, "%d", value.asIntValue());
            return program->atomizeString(buf);
        }
        default: break;
    }
    return Atom::emptyAtom(); 
}

// Local implementation to workaround ESP8266 bug
static float myfmod(float a, float b)
{
    return (a - b * floor(a / b));
}

void ExecutionUnit::run(Program* program, void (*printer)(const char*))
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static const void* dispatchTable[] {
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

        /* 0xD0 */ OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UNOP) OP(UNOP) OP(UNOP) OP(UNOP)
        /* 0xD8 */ OP(DEREF) OP(OPCODE) OP(POP) OP(STOPOP) OP(STOA) OP(OPCODE) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */ OP(STO) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xE8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(BINOP) OP(BINOP) OP(BINIOP) OP(BINIOP)
        /* 0xF0 */ OP(BINIOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINIOP)
        /* 0xF8 */ OP(BINIOP) OP(BINIOP) OP(ADD) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP)
        /* 0xFF */ OP(END)
    };
    
static_assert (sizeof(dispatchTable) == 256 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        op = static_cast<Op>(code[i++]); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    Object* obj = program->main();
    if (!obj) {
        return;
    }
    
    const Code* codeObj = obj->code();
    if (!codeObj) {
        return;
    }
    const uint8_t* code = &(codeObj->at(0));
    
    _stack.clear();
    size_t previousFrame = _stack.setLocalFrame(obj->localSize());
    size_t previousSize = _stack.size();
    int i = 0;
    
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    float floatValue;
    bool boolValue;
    uint32_t size;
    Op op;
    Value leftValue, rightValue;
    int32_t leftIntValue, rightIntValue;
    float leftFloatValue, rightFloatValue;
    Object* objectValue;
    Value returnedValue;
    int32_t returnCount;

    DISPATCH;
    
    L_UNKNOWN:
        assert(0);
        return;
    L_PUSHID:
        _stack.push(Atom::atomFromRawAtom(uintFromCode(code, i, 2)));
        i += 2;
        DISPATCH;
    L_PUSHF:
        floatValue = floatFromCode(code, i);
        _stack.push(Value(floatValue));
        i += 4;
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            uintValue = uintFromCode(code, i, size);
            i += size;
        }
        _stack.push(static_cast<int32_t>(uintValue));
        DISPATCH;
    L_PUSHSX:
        size = (static_cast<uint8_t>(op) & 0x03) + 1;
        uintValue = uintFromCode(code, i, size);
        i += size;
        _stack.push(Value(program->stringFromId(StringId::stringIdFromRawStringId(uintValue))));
        DISPATCH;
    L_PUSHO:
        uintValue = uintFromCode(code, i, 4);
        i += 4;
        _stack.push(program->objectFromObjectId(ObjectId::objectIdFromRawObjectId(uintValue)));
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        if (maskOp(op, 0x0f) == Op::PUSHL) {
            intValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            assert(size <= 2);
            intValue = intFromCode(code, i, size);
            i += size;
        }
        _stack.push(_stack.element(intValue));
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(code, i, size);
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
        i += intValue - size;
        DISPATCH;
    L_NEW:
    L_NEWX:
    L_CALL:
    L_CALLX:
        if (op == Op::CALLX || op == Op::NEWX) {
            uintValue = uintFromCode(code, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x0f);
            op = maskOp(op, 0x0f);
        }

        returnCount = call(program, uintValue, obj, op == Op::CALL || op == Op::CALLX);
    
        // Call/new is an expression. It needs to leave one item on the stack. If no values were
        // returned, push a null Value. Otherwise push the first returned value
        returnedValue = (returnCount > 0) ? _stack.top(1-returnCount) : Value();
        _stack.pop(returnCount + uintValue + 1);
        _stack.push(returnedValue);
        DISPATCH;
    L_RET:
    L_RETX:
        if (op == Op::RETX) {
            uintValue = uintFromCode(code, i, 1);
            i += 1;
        } else {
            uintValue = uintFromOp(op, 0x07);
        }
        DISPATCH;
    L_ADD:
        rightValue = _stack.top().bakeValue();
        _stack.pop();
        leftValue = _stack.top().bakeValue();
        if (leftValue.isInteger() && rightValue.isInteger()) {
            _stack.setTop(leftValue.toIntValue() + rightValue.toIntValue());
        } else if (leftValue.isNumber() && rightValue.isNumber()) {
            _stack.setTop(leftValue.toFloatValue() + rightValue.toFloatValue());
        } else {
            m8r::String s = leftValue.toStringValue();
            s += rightValue.toStringValue();
            _stack.setTop(s.c_str());
        }
        DISPATCH;
    L_UNOP:
        rightValue = _stack.top().bakeValue();
        if (rightValue.isInteger() || op != Op::UMINUS) {
            rightIntValue = rightValue.toIntValue();
            switch(op) {
                case Op::UMINUS: _stack.setTop(-rightIntValue); break;
                case Op::UNEG: _stack.setTop(~rightIntValue); break;
                case Op::UNOT: _stack.setTop((rightIntValue == 0) ? 0 : 1); break;
                default: assert(0);
            }
        } else {
            assert(op == Op::UMINUS);
            _stack.setTop(-rightValue.toFloatValue());
        }
        DISPATCH;
    L_BINIOP:
        rightIntValue = _stack.top().bakeValue().toIntValue();
        _stack.pop();
        leftIntValue = _stack.top().bakeValue().toIntValue();
        switch(op) {
            case Op::AND: _stack.setTop(leftIntValue & rightIntValue); break;
            case Op::OR: _stack.setTop(leftIntValue | rightIntValue); break;
            case Op::XOR: _stack.setTop(leftIntValue ^ rightIntValue); break;
            case Op::SHL: _stack.setTop(leftIntValue << rightIntValue); break;
            case Op::SAR: _stack.setTop(leftIntValue >> rightIntValue); break;
            case Op::SHR:
                _stack.setTop(static_cast<uint32_t>(leftIntValue) >> rightIntValue); break;
            default: assert(0); break;
        }
        DISPATCH;

    L_BINOP:
        rightValue = _stack.top().bakeValue();
        _stack.pop();
        leftValue = _stack.top().bakeValue();
        if (leftValue.isInteger() && rightValue.isInteger()) {
            leftIntValue = leftValue.toIntValue();
            rightIntValue = rightValue.toIntValue();
            switch(op) {
                case Op::LOR: _stack.setTop(leftIntValue || rightIntValue); break;
                case Op::LAND: _stack.setTop(leftIntValue && rightIntValue); break;
                case Op::EQ: _stack.setTop(leftIntValue == rightIntValue); break;
                case Op::NE: _stack.setTop(leftIntValue != rightIntValue); break;
                case Op::LT: _stack.setTop(leftIntValue < rightIntValue); break;
                case Op::LE: _stack.setTop(leftIntValue <= rightIntValue); break;
                case Op::GT: _stack.setTop(leftIntValue > rightIntValue); break;
                case Op::GE: _stack.setTop(leftIntValue >= rightIntValue); break;
                case Op::SUB: _stack.setTop(leftIntValue - rightIntValue); break;
                case Op::MUL: _stack.setTop(leftIntValue * rightIntValue); break;
                case Op::DIV: _stack.setTop(leftIntValue / rightIntValue); break;
                case Op::MOD: _stack.setTop(leftIntValue % rightIntValue); break;
                default: assert(0); break;
            }
        } else {
            leftFloatValue = leftValue.toFloatValue();
            rightFloatValue = rightValue.toFloatValue();
               
            switch(op) {
                case Op::LOR: _stack.setTop(leftFloatValue || rightFloatValue); break;
                case Op::LAND: _stack.setTop(leftFloatValue && rightFloatValue); break;
                case Op::EQ: _stack.setTop(leftFloatValue == rightFloatValue); break;
                case Op::NE: _stack.setTop(leftFloatValue != rightFloatValue); break;
                case Op::LT: _stack.setTop(leftFloatValue < rightFloatValue); break;
                case Op::LE: _stack.setTop(leftFloatValue <= rightFloatValue); break;
                case Op::GT: _stack.setTop(leftFloatValue > rightFloatValue); break;
                case Op::GE: _stack.setTop(leftFloatValue >= rightFloatValue); break;
                case Op::SUB: _stack.setTop(leftFloatValue - rightFloatValue); break;
                case Op::MUL: _stack.setTop(leftFloatValue * rightFloatValue); break;
                case Op::DIV: _stack.setTop(leftFloatValue / rightFloatValue); break;
                case Op::MOD: _stack.setTop(static_cast<float>(myfmod(leftFloatValue, rightFloatValue))); break;
                default: assert(0); break;
            }
        }
        DISPATCH;
    L_PREINC:
        _stack.top().setValue(_stack.top().toIntValue() + 1);
        DISPATCH;
    L_PREDEC:
        _stack.top().setValue(_stack.top().toIntValue() + 1);
        DISPATCH;
    L_POSTINC:
        if (!_stack.top().isLValue()) {
            if (!printError("Must have an lvalue for POSTINC", printer)) {
                return;
            }
        } else {
            if (!_stack.top().isInteger()) {
                if (!printError("Must have an integer value for POSTINC", printer)) {
                    return;
                }
            }
            leftValue = _stack.top().bakeValue();
            _stack.top().setValue(_stack.top().toIntValue() + 1);
            _stack.setTop(leftValue);
        }
        DISPATCH;
    L_POSTDEC:
        if (!_stack.top().isLValue()) {
            if (!printError("Must have an lvalue for POSTDEC", printer)) {
                return;
            }
        } else {
            if (!_stack.top().isInteger()) {
                if (!printError("Must have an integer value for POSTDEC", printer)) {
                    return;
                }
            }
            leftValue = _stack.top().bakeValue();
            _stack.top().setValue(_stack.top().toIntValue() - 1);
            _stack.setTop(leftValue);
        }
        DISPATCH;
    L_STO:
        _stack.top(-1).setValue(_stack.top());
        _stack.pop();
        DISPATCH;
    L_STOA:
        objectValue = _stack.top(-1).asObjectValue();
        if (!objectValue) {
            if (!printError("target of STOA must be an Object", printer)) {
                return;
            }
        } else {
            objectValue->appendElement(_stack.top());
        }
        _stack.pop();
        DISPATCH;
    L_POP:
        _stack.pop();
        DISPATCH;
    L_STOPOP:
        _stack.top(-1).setValue(_stack.top());
        _stack.pop();
        _stack.pop();
        DISPATCH;
    L_DEREF:
        if (!deref(program, _stack.top(-1), _stack.top())) {
            if (!printError("Deref out of range or invalid", printer)) {
                return;
            }
        }
        _stack.pop();
        DISPATCH;
    L_OPCODE:
        assert(0);
        DISPATCH;
    L_END:
        assert(_stack.size() == previousSize);
        _stack.restoreFrame(previousFrame);
        return;
}

#if SHOW_CODE
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
    s += Value::toString(uniqueID);
    s += "]\n";
    indentCode(s);
}
#endif

m8r::String ExecutionUnit::generateCodeString(const Program* program) const
{
#if SHOW_CODE
    m8r::String outputString;
    
	for (const auto& object : program->objects()) {
        if (object.value->code()) {
            outputString += generateCodeString(program, object.value, Value::toString(object.key.rawObjectId()).c_str(), _nestingLevel);
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
    static const void* dispatchTable[] {
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
    if (obj->propertyCount()) {
        for (int32_t i = 0; i < obj->propertyCount(); ++i) {
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
    }
    
    const uint8_t* code = &(obj->code()->at(0));

    // Annotate the code to add labels
    uint32_t uniqueID = 1;
    int i = 0;
    for ( ; ; ) {
        if (i >= obj->code()->size()) {
            outputString += "\n\nWENT PAST THE END OF CODE\n\n";
            return outputString;
        }
        if (obj->code()->at(i) == static_cast<uint8_t>(Op::END)) {
            break;
        }
        uint8_t c = obj->code()->at(i++);
        if (c < static_cast<uint8_t>(Op::PUSHI)) {
            int count = (c & 0x03) + 1;
            int nexti = i + count;
            
            c &= 0xfc;
            Op op = static_cast<Op>(c);
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
    
    Op op;
    
    DISPATCH;
    
    L_UNKNOWN:
        outputString += "UNKNOWN\n";
        DISPATCH;
    L_PUSHID:
        preamble(outputString, i - 1);
        strValue = program->stringFromRawAtom(uintFromCode(code, i, 2));
        i += 2;
        outputString += "ID(";
        outputString += strValue.c_str();
        outputString += ")\n";
        DISPATCH;
    L_PUSHF:
        preamble(outputString, i - 1);
        uintValue = uintFromCode(code, i, 4);
        i += 4;
        outputString += "FLT(";
        outputString += Value::toString(*(reinterpret_cast<float*>(&uintValue)));
        outputString += ")\n";
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        preamble(outputString, i - 1);
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            uintValue = uintFromCode(code, i, size);
            i += size;
        }
        outputString += "INT(";
        outputString += Value::toString(uintValue);
        outputString += ")\n";
        DISPATCH;
    L_PUSHSX:
        preamble(outputString, i - 1);
        size = (static_cast<uint8_t>(op) & 0x03) + 1;
        uintValue = uintFromCode(code, i, size);
        i += size;
        outputString += "STR(\"";
        outputString += program->stringFromId(StringId::stringIdFromRawStringId(uintValue));
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
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            uintValue = uintFromCode(code, i, size);
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
    
    OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UPLUS) OP(UMINUS) OP(UNOT) OP(UNEG)
    OP(DEREF) OP(DEL) OP(POP) OP(STOPOP)
    
    OP(STOA)
    OP(STOO)

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

