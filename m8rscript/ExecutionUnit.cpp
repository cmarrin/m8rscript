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

#include "Float.h"
#include "Parser.h"
#include "SystemInterface.h"
#include <cassert>

using namespace m8r;

bool ExecutionUnit::printError(const char* s) const
{
    ++_nerrors;
    if (_system) {
        _system->print("Runtime error: ");
        _system->print(s);
        _system->print("\n");
    }
    if (_nerrors >= 10) {
        if (_system) {
            _system->print("\n\nToo many runtime errors, exiting...\n");
            _terminate = true;
        }
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
        callee = deref(program, nullptr, callee);
    }
    
    // FIXME: when error occurs (return < 0) log an error or something
    int32_t returnCount = callee.call(program, this, nparams);
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
    
    if (!obj && derefValue.type() == Value::Type::Id) {
        // First see if this is a callable property of the main function
        // FIXME: Need to walk up the function chain
        Atom name = derefValue.asIdValue();
        Object* testObj = program->main();
        for (int32_t i = 0; i < testObj->propertyCount(); ++i) {
            const Value& value = testObj->property(i);
            if (value.isNone()) {
                continue;
            }
            if (value.asObjectValue() && value.asObjectValue()->code()) {
                if (testObj->propertyName(i) == name) {
                    return value;
                }
            }
        }
    }
    
    if (!obj) {
        obj = program->global();
    }

    int32_t index = obj->propertyIndex(propertyNameFromValue(program, derefValue), true);
    if (index < 0) {
        String s = "no property '";
        s += derefValue.toStringValue();
        s += "' in '";
        s += obj->typeName();
        s += "' Object";
        printError(s.c_str());
        return Value();
    }
    return obj->propertyRef(index);
}

bool ExecutionUnit::deref(Program* program, Value& objectValue, const Value& derefValue)
{
    if (objectValue.type() == Value::Type::Id) {
        objectValue = deref(program, program->global(), objectValue);
        if (objectValue.isNone()) {
            String s = "    '";
            s += derefValue.toStringValue();
            s += "' property does not exist";
            return printError(s.c_str());
        }
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
        Value elementRef = obj->elementRef(derefValue.toIntValue());
        if (!elementRef.isNone()) {
            objectValue = elementRef;
            return true;
        }
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
            return program->atomizeString(Value::toString(value.asIntValue()).c_str());
        }
        case Value::Type::Float: {
            return program->atomizeString(Value::toString(value.asFloatValue()).c_str());
        }
        default: break;
    }
    return Atom();
}

void ExecutionUnit::run(Program* program)
{
    _terminate = false;
    _nerrors = 0;
    _stack.clear();
    run(program, program->main(), 0);
}

int32_t ExecutionUnit::run(Program* program, Object* obj, uint32_t nparams)
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
        /* 0xC0 */      OP(PUSHLITA) OP(PUSHLITO) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xC8 */      OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)

        /* 0xD0 */ OP(PREINC) OP(PREDEC) OP(POSTINC) OP(POSTDEC) OP(UNOP) OP(UNOP) OP(UNOP) OP(UNOP)
        /* 0xD8 */ OP(DEREF) OP(OPCODE) OP(POP) OP(STOPOP) OP(STOA) OP(STOO) OP(UNKNOWN) OP(UNKNOWN)
        /* 0xE0 */ OP(STO) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE)
        /* 0xE8 */ OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(OPCODE) OP(BINIOP) OP(BINIOP) OP(BINIOP) OP(BINIOP)
        /* 0xF0 */ OP(BINIOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINIOP)
        /* 0xF8 */ OP(BINIOP) OP(BINIOP) OP(ADD) OP(BINOP) OP(BINOP) OP(BINOP) OP(BINOP)
        /* 0xFF */ OP(END)
    };
    
static_assert (sizeof(dispatchTable) == 256 * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        if (_terminate) { \
            goto L_END; \
        } \
        op = static_cast<Op>(code[i++]); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    if (!obj) {
        return -1;
    }
    
    const Code* codeObj = obj->code();
    if (!codeObj) {
        return -1;
    }
    if (!codeObj->size()) {
        return -1;
    }
    
    const uint8_t* code = &(codeObj->at(0));
    
    size_t previousFrame = _stack.setLocalFrame(nparams, obj->localSize());
    size_t previousSize = _stack.size();
    int i = 0;
    
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    Float floatValue;
    bool boolValue;
    uint32_t size;
    Op op;
    Value leftValue, rightValue;
    int32_t leftIntValue, rightIntValue;
    Float leftFloatValue, rightFloatValue;
    Object* objectValue;
    Value returnedValue;
    int32_t callReturnCount;
    int32_t retCount = 0;

    DISPATCH;
    
    L_UNKNOWN:
        assert(0);
        return 0;
    L_PUSHID:
        _stack.push(Atom(uintFromCode(code, i, 2)));
        i += 2;
        DISPATCH;
    L_PUSHF:
        size = sizeFromOp(op);
        floatValue = Float::make(uintFromCode(code, i, sizeFromOp(op)));
        _stack.push(Value(floatValue));
        i += size;
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = sizeFromOp(op);
            uintValue = uintFromCode(code, i, size);
            i += size;
        }
        _stack.push(static_cast<int32_t>(uintValue));
        DISPATCH;
    L_PUSHSX:
        size = sizeFromOp(op);
        uintValue = uintFromCode(code, i, size);
        i += size;
        _stack.push(Value(program->stringFromStringLiteral(StringLiteral(uintValue))));
        DISPATCH;
    L_PUSHO:
        uintValue = uintFromCode(code, i, 4);
        i += 4;
        _stack.push(program->objectFromObjectId(ObjectId(uintValue)));
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
        _stack.push(_stack.elementRef(intValue));
        DISPATCH;
    L_PUSHLITA:
        _stack.push(new Array());
        DISPATCH;
    L_PUSHLITO:
        _stack.push(new MaterObject());
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

        callReturnCount = call(program, uintValue, obj, op == Op::CALL || op == Op::CALLX);
    
        // Call/new is an expression. It needs to leave one item on the stack. If no values were
        // returned, push a null Value. Otherwise push the first returned value
        
        // On return the stack still has the passed params and the callee. Pop those too
        returnedValue = (callReturnCount > 0) ? _stack.top(1-callReturnCount) : Value();
        _stack.pop(callReturnCount + uintValue + 1);
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
        retCount = uintValue;
        goto L_END;
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
            case Op::LOR: _stack.setTop(leftIntValue || rightIntValue); break;
            case Op::LAND: _stack.setTop(leftIntValue && rightIntValue); break;
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
                case Op::EQ: _stack.setTop(leftFloatValue == rightFloatValue); break;
                case Op::NE: _stack.setTop(leftFloatValue != rightFloatValue); break;
                case Op::LT: _stack.setTop(leftFloatValue < rightFloatValue); break;
                case Op::LE: _stack.setTop(leftFloatValue <= rightFloatValue); break;
                case Op::GT: _stack.setTop(leftFloatValue > rightFloatValue); break;
                case Op::GE: _stack.setTop(leftFloatValue >= rightFloatValue); break;
                case Op::SUB: _stack.setTop(leftFloatValue - rightFloatValue); break;
                case Op::MUL: _stack.setTop(leftFloatValue * rightFloatValue); break;
                case Op::DIV: _stack.setTop(leftFloatValue / rightFloatValue); break;
                case Op::MOD: _stack.setTop(leftFloatValue % rightFloatValue); break;
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
            printError("Must have an lvalue for POSTINC");
        } else {
            if (!_stack.top().isInteger()) {
                printError("Must have an integer value for POSTINC");
            }
            leftValue = _stack.top().bakeValue();
            _stack.top().setValue(_stack.top().toIntValue() + 1);
            _stack.setTop(leftValue);
        }
        DISPATCH;
    L_POSTDEC:
        if (!_stack.top().isLValue()) {
            printError("Must have an lvalue for POSTDEC");
        } else {
            if (!_stack.top().isInteger()) {
                printError("Must have an integer value for POSTDEC");
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
            printError("target of STOA must be an Object");
        } else {
            objectValue->appendElement(_stack.top());
        }
        _stack.pop();
        DISPATCH;
    L_STOO:
        objectValue = _stack.top(-2).asObjectValue();
        if (!objectValue) {
            printError("target of STOO must be an Object");
        } else {
            Atom name = propertyNameFromValue(program, _stack.top(-1));
            if (!name) {
                printError("Object literal property name must be id, string, integer or float");
            } else {
                int32_t index = objectValue->addProperty(name);
                if (index < 0) {
                    String s = "Invalid property '";
                    s += Program::stringFromAtom(name);
                    s += "' for Object literal";
                    printError(s.c_str());
                }
                objectValue->setProperty(index, _stack.top());
            }
        }
        _stack.pop();
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
        deref(program, _stack.top(-1), _stack.top());
        _stack.pop();
        DISPATCH;
    L_OPCODE:
        assert(0);
        DISPATCH;
    L_END:
        if (!_terminate) {
            assert(_stack.size() == previousSize + retCount);
        }
        _stack.restoreFrame(previousFrame, obj->localSize());
        return retCount;
}
