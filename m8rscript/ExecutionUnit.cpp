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
#include "Array.h"
#include "SystemInterface.h"

using namespace m8r;

bool ExecutionUnit::printError(const char* s) const
{
    ++_nerrors;
    if (_system) {
        _system->printf("Runtime error: %s\n", s);
    }
    if (_nerrors >= 10) {
        if (_system) {
            _system->printf("\n\nToo many runtime errors, exiting...\n");
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

CallReturnValue ExecutionUnit::call(Program* program, uint32_t nparams, Object* obj, bool isNew)
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
    
    return callee.call(this, nparams);
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
        Object* testObj = program;
        for (uint32_t i = 0; i < testObj->propertyCount(); ++i) {
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

    int32_t index = obj->propertyIndex(propertyNameFromValue(program, derefValue));
    if (index < 0) {
        String s = "no property '";
        s += derefValue.toStringValue(_program);
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
            s += derefValue.toStringValue(_program);
            s += "' property does not exist";
            return printError(s.c_str());
        }
        return deref(program, objectValue, derefValue);
    }
    
    if (objectValue.type() == Value::Type::PropertyRef) {
        objectValue = objectValue.appendPropertyRef(derefValue);
        if (objectValue.isNone()) {
            String s = "'";
            s += derefValue.toStringValue(_program);
            s += "' property does not exist";
            return printError(s.c_str());
        }
        return true;
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
    int32_t index = obj->propertyIndex(propertyNameFromValue(program, derefValue));
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

void ExecutionUnit::startExecution(Program* program)
{
    _terminate = false;
    _nerrors = 0;
    _pc = 0;
    _program = program;
    _object = program;
    _stack.clear();
    _stack.setLocalFrame(0, _object ? _object->localSize() : 0);
}

void ExecutionUnit::startFunction(Function* function, uint32_t nparams)
{
    _stack.push(Value(static_cast<uint32_t>(_stack.setLocalFrame(nparams, _object->localSize())), Value::Type::PreviousFrame));
    _stack.push(Value(_pc, Value::Type::PreviousPC));
    _pc = 0;
    _stack.push(Value(_object, Value::Type::PreviousObject));
    _object = function;
}

int32_t ExecutionUnit::continueExecution()
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static const void* ICACHE_RODATA_ATTR ICACHE_STORE_ATTR dispatchTable[] {
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
        /* 0xC0 */      OP(PUSHLITA) OP(PUSHLITO)  OP(PUSHTRUE) OP(PUSHFALSE) OP(PUSHNULL) OP(UNKNOWN) OP(UNKNOWN) OP(UNKNOWN)
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

static const uint16_t YieldCount = 2000;

    #undef DISPATCH
    #define DISPATCH { \
        if (_terminate || _pc >= _codeSize) { \
            goto L_END; \
        } \
        if (--yieldCounter == 0) { \
            yieldCounter = YieldCount; \
            return 0; \
        } \
        op = static_cast<Op>(_code[_pc++]); \
        goto *dispatchTable[static_cast<uint8_t>(op)]; \
    }
    
    if (!_program) {
        return -1;
    }

    uint16_t yieldCounter = YieldCount;
    updateCodePointer();
    
    m8r::String strValue;
    uint32_t uintValue;
    int32_t intValue;
    Float floatValue;
    bool boolValue;
    uint32_t size;
    Op op = Op::END;
    Value leftValue, rightValue;
    int32_t leftIntValue, rightIntValue;
    Float leftFloatValue, rightFloatValue;
    Object* objectValue;
    Value returnedValue;
    CallReturnValue callReturnValue;
    
    DISPATCH;
    
    L_UNKNOWN:
        assert(0);
        return -1;
    L_PUSHID:
        _stack.push(Atom(uintFromCode(_code, _pc, 2)));
        _pc += 2;
        DISPATCH;
    L_PUSHF:
        size = sizeFromOp(op);
        floatValue = Float::make(uintFromCode(_code, _pc, sizeFromOp(op)));
        _stack.push(Value(floatValue));
        _pc += size;
        DISPATCH;
    L_PUSHI:
    L_PUSHIX:
        if (maskOp(op, 0x0f) == Op::PUSHI) {
            uintValue = uintFromOp(op, 0x0f);
        } else {
            size = sizeFromOp(op);
            uintValue = uintFromCode(_code, _pc, size);
            _pc += size;
        }
        _stack.push(static_cast<int32_t>(uintValue));
        DISPATCH;
    L_PUSHSX:
        size = sizeFromOp(op);
        uintValue = uintFromCode(_code, _pc, size);
        _pc += size;
        _stack.push(Value(_program->stringFromStringLiteral(StringLiteral(uintValue))));
        DISPATCH;
    L_PUSHO:
        uintValue = uintFromCode(_code, _pc, 4);
        _pc += 4;
        _stack.push(_program->objectFromObjectId(ObjectId(uintValue)));
        DISPATCH;
    L_PUSHL:
    L_PUSHLX:
        if (maskOp(op, 0x0f) == Op::PUSHL) {
            intValue = uintFromOp(op, 0x0f);
        } else {
            size = (static_cast<uint8_t>(op) & 0x03) + 1;
            assert(size <= 2);
            intValue = intFromCode(_code, _pc, size);
            _pc += size;
        }
        _stack.push(_stack.elementRef(intValue));
        DISPATCH;
    L_PUSHLITA:
        _stack.push(new Array(_program));
        DISPATCH;
    L_PUSHLITO:
        _stack.push(new MaterObject());
        DISPATCH;
    L_PUSHTRUE:
        _stack.push(true);
        DISPATCH;
    L_PUSHFALSE:
        _stack.push(false);
        DISPATCH;
    L_PUSHNULL:
        _stack.push(Value());
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        size = intFromOp(op, 0x01) + 1;
        intValue = intFromCode(_code, _pc, size);
        op = maskOp(op, 0x01);
        _pc += size;
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
        _pc += intValue - size;
        DISPATCH;
    L_NEW:
    L_NEWX:
    L_CALL:
    L_CALLX:
        if (op == Op::CALLX || op == Op::NEWX) {
            uintValue = uintFromCode(_code, _pc, 1);
            _pc += 1;
        } else {
            uintValue = uintFromOp(op, 0x0f);
            op = maskOp(op, 0x0f);
        }

        callReturnValue = call(_program, uintValue, _object, op == Op::CALL || op == Op::CALLX);

        // If the callReturnValue is FunctionStart it means we've called a Function and it just
        // setup the EU to execute it. In that case just continue
        if (callReturnValue.isFunctionStart()) {
            updateCodePointer();
            DISPATCH;
        }

        // Call/new is an expression. It needs to leave one item on the stack. If no values were
        // returned, push a null Value. Otherwise push the first returned value
        
        // On return the stack still has the passed params and the callee. Pop those too
        returnedValue = Value();
        if (callReturnValue.isReturnCount() && callReturnValue.returnCount() > 0) {
            returnedValue = _stack.top(1 - callReturnValue.returnCount());
            _stack.pop(callReturnValue.returnCount());
        }
        _stack.pop(uintValue + 1);
        _stack.push(returnedValue);
        if (callReturnValue.isMsDelay()) {
            return callReturnValue.msDelay();
        }
        DISPATCH;
    L_RET:
    L_RETX:
    L_END:
        if (_terminate || op == Op::END) {
            if (_terminate || _program == _object) {
                // We've hit the end of the program
                if (!_terminate) {
                    assert(_stack.validateFrame(0, _program->localSize()));
                }
                _stack.clear();
                return -1;
            }
            callReturnValue = CallReturnValue();
        }
        else if (op == Op::RETX) {
            callReturnValue = CallReturnValue(CallReturnValue::Type::ReturnCount, uintFromCode(_code, _pc, 1));
            _pc += 1;
        } else {
            callReturnValue = CallReturnValue(CallReturnValue::Type::ReturnCount, uintFromOp(op, 0x0f));
        }
        
        // The stack now contains:
        //
        //        tos-> [retCount Values]
        //              <previous Object>
        //              <previous PC>
        //              <previous Frame>
        //              [locals]
        //      frame-> [params]
        //              <callee>
        returnedValue = (callReturnValue.isReturnCount() && callReturnValue.returnCount() > 0) ? _stack.top(1 - callReturnValue.returnCount()) : Value();
        _stack.pop(callReturnValue.returnCount());
        
        assert(_stack.top().type() == Value::Type::PreviousObject);
        _object = _stack.top().asObjectValue();
        _stack.pop();
        
        assert(_stack.top().type() == Value::Type::PreviousPC);
        _pc = _stack.top().asUIntValue();
        _stack.pop();
        
        assert(_stack.top().type() == Value::Type::PreviousFrame);
        _stack.restoreFrame(_stack.top().asUIntValue());
    
        // Pop callee
        _stack.pop();
        
        updateCodePointer();
       
        _stack.push(returnedValue);
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
            m8r::String s = leftValue.toStringValue(_program);
            s += rightValue.toStringValue(_program);
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
        if (!_stack.top(-1).setValue(_stack.top())) {
            String s = "Attempted to assign to nonexistant variable '";
                    s += _stack.top(-1).toStringValue(_program);
                    s += "'";
                    printError(s.c_str());
        }
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
            Atom name = propertyNameFromValue(_program, _stack.top(-1));
            if (!name) {
                printError("Object literal property name must be id, string, integer or float");
            } else {
                int32_t index = objectValue->addProperty(name);
                if (index < 0) {
                    String s = "Invalid property '";
                    s += _program->stringFromAtom(name);
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
        deref(_program, _stack.top(-1), _stack.top());
        _stack.pop();
        DISPATCH;
    L_OPCODE:
        assert(0);
        DISPATCH;
}
