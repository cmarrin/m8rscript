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

bool ExecutionUnit::checkTooManyErrors() const
{
    if (++_nerrors >= 10) {
        if (_system) {
            _system->printf(ROMSTR("\n\nToo many runtime errors, exiting...\n"));
            _terminate = true;
        }
        return false;
    }
    return true;
}

bool ExecutionUnit::printError(const char* format, ...) const
{
    va_list args;
    va_start(args, format);
    Error::vprintError(_system, Error::Code::RuntimeError, format, args);
    return checkTooManyErrors();
}

Value* ExecutionUnit::valueFromId(Atom id, const Object* obj) const
{
    // Start at the current object and walk up the chain
    return nullptr;
}

Value ExecutionUnit::derefId(Atom atom)
{
    if (!atom) {
        printError(ROMSTR("Value in PUSHREFK must be an Atom"));
        return Value();
    }

    // This atom is a property of the Program or Global objects
    Object* object = _program;
    int32_t index = _program->propertyIndex(atom);
    if (index < 0) {
        object = _program->global();
        index = _program->global()->propertyIndex(atom);
        if (index < 0) {
            printError(ROMSTR("'%s' property does not exist in global scope"), _program->stringFromAtom(atom).c_str());
            return Value();
        }
    }
    
    return object->propertyRef(index);
}

void ExecutionUnit::startExecution(Program* program)
{
    _terminate = false;
    _nerrors = 0;
    _pc = 0;
    _program = program;
    _object = _program->objectId();
    Object* obj = _program->obj(_object);
    if (!obj->isFunction()) {
        _terminate = true;
        _functionPointer = nullptr;
        _stack.clear();
        return;
    }
    _functionPointer =  static_cast<Function*>(obj);
    _program->setStack(&_stack);
    _stack.clear();
    _stack.setLocalFrame(0, _object ? _functionPointer->localSize() : 0);
}

void ExecutionUnit::startFunction(ObjectId function, uint32_t nparams)
{
    Object* functionPointer = _program->obj(function);
    assert(functionPointer->isFunction());
    
    _stack.push(Value(static_cast<uint32_t>(_stack.setLocalFrame(nparams, functionPointer->localSize())), Value::Type::PreviousFrame));
    _stack.push(Value(_pc, Value::Type::PreviousPC));
    _pc = 0;
    _stack.push(Value(_object, Value::Type::PreviousObject));
    _object = function;
    _functionPointer =  static_cast<Function*>(functionPointer);
}

int32_t ExecutionUnit::continueExecution()
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static const void* ICACHE_RODATA_ATTR ICACHE_STORE_ATTR dispatchTable[] {
        /* 0x00 */ OP(MOVE) OP(LOADREFK) OP(LOADLITA) OP(LOADLITO)
        /* 0x04 */ OP(LOADPROP) OP(LOADELT) OP(STOPROP) OP(STOELT)
        /* 0x08 */ OP(APPENDELT) OP(APPENDPROP) OP(LOADTRUE) OP(LOADFALSE)
        /* 0x0C */ OP(LOADNULL) OP(PUSH) OP(POP) OP(UNKNOWN)

        /* 0x10 */ OP(BINIOP) OP(BINIOP) OP(BINIOP) OP(BINIOP)
        /* 0x14 */ OP(BINIOP) OP(BINOP) OP(BINOP) OP(BINOP)
        /* 0x18 */ OP(BINOP) OP(BINOP) OP(BINOP) OP(BINIOP)
        /* 0x1C */ OP(BINIOP) OP(BINIOP) OP(ADD) OP(BINOP)
        
        /* 0x20 */ OP(BINOP)  OP(BINOP)  OP(BINOP)  OP(DEREF)
        /* 0x24 */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x2c */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x30 */ OP(UNOP)  OP(UNOP)  OP(UNOP)  OP(PREINC)
        /* 0x34 */ OP(PREDEC)  OP(POSTINC)  OP(POSTDEC)  OP(CALL)
        /* 0x38 */ OP(NEW)  OP(JMP)  OP(JT)  OP(JF)        
        /* 0x3c */ OP(UNKNOWN) OP(RET) OP(END) OP(UNKNOWN)
    };
 
//static_assert (sizeof(dispatchTable) == (1 << 6) * sizeof(void*), "Dispatch table is wrong size");

static const uint16_t YieldCount = 10000;

    #undef DISPATCH
    #define DISPATCH { \
        if (_terminate || _pc >= _codeSize) { \
            goto L_END; \
        } \
        if (--yieldCounter == 0) { \
            yieldCounter = YieldCount; \
            return 0; \
        } \
        inst = _code[_pc++]; \
        op = static_cast<Op>(inst.op); \
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
    Value leftValue, rightValue;
    int32_t leftIntValue, rightIntValue;
    Float leftFloatValue, rightFloatValue;
    Object* objectValue;
    ObjectId objectId;
    Value returnedValue;
    CallReturnValue callReturnValue;
    size_t localsToPop;

    Instruction inst;
    Op op = Op::END;
    
    DISPATCH;
    
    L_UNKNOWN:
        assert(0);
        return -1;
    L_RET:
    L_RETX:
    L_END:
        if (_terminate || op == Op::END) {
            if (_terminate || _program == _functionPointer) {
                // We've hit the end of the program
                if (!_terminate) {
                    assert(_stack.validateFrame(0, _program->localSize()));
                }
                _stack.clear();
                return -1;
            }
            callReturnValue = CallReturnValue();
        }
        else {
            callReturnValue = CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
        }
        
        // The stack now contains:
        //
        //        tos-> [retCount Values]
        //              <previous Object>
        //              <previous PC>
        //              <previous Frame>
        //              [locals]
        //      frame-> [params]
        returnedValue = (callReturnValue.isReturnCount() && callReturnValue.returnCount() > 0) ? _stack.top(1 - callReturnValue.returnCount()) : Value();
        _stack.pop(callReturnValue.returnCount());
        
        localsToPop = _functionPointer->localSize();
        
        assert(_stack.top().type() == Value::Type::PreviousObject);
        _stack.pop(leftValue);
        _object = leftValue.asObjectIdValue();
        objectValue = _program->obj(_object);
        assert(objectValue->isFunction());
        _functionPointer = static_cast<Function*>(objectValue);
        
        assert(_stack.top().type() == Value::Type::PreviousPC);
        _stack.pop(leftValue);
        _pc = leftValue.asUIntValue();
        
        assert(_stack.top().type() == Value::Type::PreviousFrame);
        _stack.pop(leftValue);
        _stack.restoreFrame(leftValue.asUIntValue(), localsToPop);
        
        updateCodePointer();
    
        _stack.push(returnedValue);
        DISPATCH;
    L_MOVE:
        _stack.setInFrame(inst.ra, regOrConst(inst.rb));
        DISPATCH;
    L_LOADREFK:
        _stack.setInFrame(inst.ra, derefId(regOrConst(inst.rb).asIdValue()));
        DISPATCH;
    L_LOADLITA:
    L_LOADLITO:
        if (op == Op::LOADLITA) {
            objectValue = new Array(_program);
        } else {
            objectValue = new MaterObject();
        }
        objectId = _program->addObject(objectValue);
        objectValue->setObjectId(objectId);
        _stack.setInFrame(inst.ra, objectId);
        DISPATCH;
    L_LOADPROP:
    L_LOADELT:
        leftValue = regOrConst(inst.rb);
        rightValue = regOrConst(inst.rc);
        if (!leftValue.deref(this, rightValue, false)) {
            if (checkTooManyErrors()) {
                return -1;
            }
        }
        _stack.setInFrame(inst.ra, leftValue);
        DISPATCH;
    L_STOPROP:
        // ra - Object
        // rb - prop
        // rc - value to store
        objectValue = _program->obj(regOrConst(inst.ra).asObjectIdValue());
        if (!objectValue) {
            printError(ROMSTR("Null Object for STOPROP"));
            DISPATCH;
        }
        leftValue = regOrConst(inst.ra).bake(this).deref(this, regOrConst(inst.rb).bake(this), false);
        leftValue.setValue(this, regOrConst(inst.rc).bake(this));
        DISPATCH;
    L_STOELT:
        DISPATCH;
    L_APPENDELT:
        objectValue = _program->obj(regOrConst(inst.ra).asObjectIdValue());
        if (!objectValue) {
            printError(ROMSTR("Null Array for APPENDELT"));
            DISPATCH;
        }
        rightValue = regOrConst(inst.rb).bake(this);
        objectValue->appendElement(this, rightValue);
        DISPATCH;
    L_APPENDPROP:
        // ra - Object
        // rb - prop
        // rc - value to store
        leftValue = regOrConst(inst.ra).bake(this);
        leftValue.deref(this, regOrConst(inst.rb).bake(this), true);
        leftValue.setValue(this, regOrConst(inst.rc).bake(this));
        DISPATCH;
    L_LOADTRUE:
        _stack.setInFrame(inst.ra, true);
        DISPATCH;
    L_LOADFALSE:
        _stack.setInFrame(inst.ra, false);
        DISPATCH;
    L_LOADNULL:
        _stack.setInFrame(inst.ra, Value());
        DISPATCH;
    L_PUSH:
        _stack.push(regOrConst(inst.rn));
        DISPATCH;
    L_POP:
        _stack.setInFrame(inst.ra, _stack.top());
        _stack.pop();
        DISPATCH;
    L_DEREF:
        leftValue = regOrConst(inst.rb);
        if (!leftValue.deref(this, regOrConst(inst.rc), false)) {
            if (checkTooManyErrors()) {
                return -1;
            }
        }
        _stack.setInFrame(inst.ra, leftValue);
        DISPATCH;
    L_BINIOP:
        leftIntValue = regOrConst(inst.rb).toIntValue(this);
        rightIntValue = regOrConst(inst.rc).toIntValue(this);
        switch(op) {
            case Op::LOR: leftIntValue = leftIntValue || rightIntValue; break;
            case Op::LAND: leftIntValue = leftIntValue && rightIntValue; break;
            case Op::AND: leftIntValue &= rightIntValue; break;
            case Op::OR: leftIntValue |= rightIntValue; break;
            case Op::XOR: leftIntValue ^= rightIntValue; break;
            case Op::SHL: leftIntValue <<= rightIntValue; break;
            case Op::SAR: leftIntValue >>= rightIntValue; break;
            case Op::SHR: leftIntValue = static_cast<uint32_t>(leftIntValue) >> rightIntValue; break;
            default: assert(0); break;
        }
        _stack.setInFrame(inst.ra, leftIntValue);
        DISPATCH;
    L_BINOP:
        leftValue = regOrConst(inst.rb).bake(this);
        rightValue = regOrConst(inst.rc).bake(this);
        if (leftValue.isInteger() && rightValue.isInteger()) {
            leftIntValue = leftValue.toIntValue(this);
            rightIntValue = rightValue.toIntValue(this);
            switch(op) {
                case Op::EQ: leftIntValue = leftIntValue == rightIntValue; break;
                case Op::NE: leftIntValue = leftIntValue != rightIntValue; break;
                case Op::LT: leftIntValue = leftIntValue < rightIntValue; break;
                case Op::LE: leftIntValue = leftIntValue <= rightIntValue; break;
                case Op::GT: leftIntValue = leftIntValue > rightIntValue; break;
                case Op::GE: leftIntValue = leftIntValue >= rightIntValue; break;
                case Op::SUB: leftIntValue -= rightIntValue; break;
                case Op::MUL: leftIntValue *= rightIntValue; break;
                case Op::DIV: leftIntValue /= rightIntValue; break;
                case Op::MOD: leftIntValue %= rightIntValue; break;
                default: assert(0); break;
            }
            _stack.setInFrame(inst.ra, leftIntValue);
        } else {
            leftFloatValue = leftValue.toFloatValue(this);
            rightFloatValue = rightValue.toFloatValue(this);
            switch(op) {
                case Op::EQ: leftFloatValue = leftFloatValue == rightFloatValue; break;
                case Op::NE: leftFloatValue = leftFloatValue != rightFloatValue; break;
                case Op::LT: leftFloatValue = leftFloatValue < rightFloatValue; break;
                case Op::LE: leftFloatValue = leftFloatValue <= rightFloatValue; break;
                case Op::GT: leftFloatValue = leftFloatValue > rightFloatValue; break;
                case Op::GE: leftFloatValue = leftFloatValue >= rightFloatValue; break;
                case Op::SUB: leftFloatValue -= rightFloatValue; break;
                case Op::MUL: leftFloatValue *= rightFloatValue; break;
                case Op::DIV: leftFloatValue /= rightFloatValue; break;
                case Op::MOD: leftFloatValue %= rightFloatValue; break;
                default: assert(0); break;
            }
            _stack.setInFrame(inst.ra, leftFloatValue);
        }
        DISPATCH;
    L_ADD:
        leftValue = regOrConst(inst.rb).bake(this);
        rightValue = regOrConst(inst.rc).bake(this);

        if (leftValue.isInteger() && rightValue.isInteger()) {
            _stack.setInFrame(inst.ra, leftValue.toIntValue(this) + rightValue.toIntValue(this));
        } else if (leftValue.isNumber() && rightValue.isNumber()) {
            _stack.setInFrame(inst.ra, leftValue.toFloatValue(this) + rightValue.toFloatValue(this));
        } else {
            StringId stringId = _program->createString();
            String& s = _program->str(stringId);
            s = leftValue.toStringValue(this);
            s += rightValue.toStringValue(this);
            _stack.setInFrame(inst.ra, stringId);
        }
        DISPATCH;
    L_UNOP:
        leftValue = regOrConst(inst.rb).bake(this);
        if (leftValue.isInteger() || op != Op::UMINUS) {
            leftIntValue = leftValue.toIntValue(this);
            switch(op) {
                case Op::UMINUS: leftIntValue = -leftIntValue; break;
                case Op::UNEG: leftIntValue = ~leftIntValue; break;
                case Op::UNOT: leftIntValue = (leftIntValue == 0) ? 0 : 1; break;
                default: assert(0);
            }
            _stack.setInFrame(inst.ra, leftIntValue);
        } else {
            leftFloatValue = -leftValue.toFloatValue(this);
            _stack.setInFrame(inst.ra, leftFloatValue);
        }
        DISPATCH;
    L_PREINC:
        _stack.setInFrame(inst.rb, regOrConst(inst.rb).toIntValue(this) + 1);
        _stack.setInFrame(inst.ra, regOrConst(inst.rb));
        DISPATCH;
    L_PREDEC:
        _stack.setInFrame(inst.rb, regOrConst(inst.rb).toIntValue(this) - 1);
        _stack.setInFrame(inst.ra, regOrConst(inst.rb));
        DISPATCH;
    L_POSTINC:
        _stack.setInFrame(inst.ra, regOrConst(inst.rb));
        _stack.setInFrame(inst.rb, regOrConst(inst.rb).toIntValue(this) + 1);
        DISPATCH;
    L_POSTDEC:
        _stack.setInFrame(inst.ra, regOrConst(inst.rb));
        _stack.setInFrame(inst.rb, regOrConst(inst.rb).toIntValue(this) - 1);
        DISPATCH;
    L_NEW:
    L_CALL:
        uintValue = inst.un;
        callReturnValue = regOrConst(inst.rn).call(this, uintValue);

        // If the callReturnValue is FunctionStart it means we've called a Function and it just
        // setup the EU to execute it. In that case just continue
        if (callReturnValue.isFunctionStart()) {
            updateCodePointer();
            DISPATCH;
        }

        // Call/new is an expression. It needs to leave one item on the stack. If no values were
        // returned, push a null Value. Otherwise push the first returned value
        
        // On return the stack still has the passed params. Pop those too
        returnedValue = Value();
        if (callReturnValue.isReturnCount() && callReturnValue.returnCount() > 0) {
            returnedValue = _stack.top(1 - callReturnValue.returnCount());
            _stack.pop(callReturnValue.returnCount());
        }
        _stack.pop(uintValue);
        _stack.push(returnedValue);
        if (callReturnValue.isMsDelay()) {
            return callReturnValue.msDelay();
        }
        DISPATCH;
    L_JMP:
    L_JT:
    L_JF:
        intValue = inst.sn;
        if (op != Op::JMP) {
            boolValue = regOrConst(inst.rn).toBoolValue(this);
            if (op == Op::JT) {
                boolValue = !boolValue;
            }
            if (boolValue) {
                DISPATCH;
            }
        }
        _pc += intValue - 1;
        DISPATCH;
}
