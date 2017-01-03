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
        SystemInterface::shared()->printf(ROMSTR("\n\nToo many runtime errors, (%d) exiting...\n"), _nerrors);
        _terminate = true;
        return false;
    }
    return true;
}

bool ExecutionUnit::printError(const char* format, ...) const
{
    va_list args;
    va_start(args, format);
    Error::vprintError(Error::Code::RuntimeError, -1, format, args);
    return checkTooManyErrors();
}

void ExecutionUnit::objectError(const char* s) const
{
    printError(ROMSTR("Value must be Object for %s"), s);
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
    Value value = _program->property(this, atom);
    if (!value) {
        value = _program->global()->property(this, atom);
        if (!value) {
            printError(ROMSTR("'%s' property does not exist in global scope"), _program->stringFromAtom(atom).c_str());
            return Value();
        }
    }
    
    return value;
}

void ExecutionUnit::startExecution(Program* program)
{
    if (!program) {
        _terminate = true;
        _functionPtr = nullptr;
        _constantsPtr = nullptr;
        _stack.clear();
        return;
    }
    _terminate = false;
    _nerrors = 0;
    _pc = 0;
    _program = program;
    _object = _program->objectId();
    _functionPtr =  _program;
    _constantsPtr = _functionPtr->constantsPtr();
    _program->setStack(&_stack);
    _stack.clear();
    _stack.setLocalFrame(0, 0, _functionPtr->localSize());
    _framePtr =_stack.framePtr();
    _formalParamCount = 0;
    _actualParamCount = 0;
    _localOffset = 0;
}

void ExecutionUnit::startFunction(ObjectId function, uint32_t nparams)
{
    Object* p = _program->obj(function);
    assert(p->isFunction());
    Function* functionPtr = static_cast<Function*>(p);
    _functionPtr =  static_cast<Function*>(functionPtr);
    _constantsPtr = _functionPtr->constantsPtr();
    _formalParamCount = _functionPtr->formalParamCount();
    uint32_t prevActualParamCount = _actualParamCount;
    _actualParamCount = nparams;
    _localOffset = ((_formalParamCount < _actualParamCount) ? _actualParamCount : _formalParamCount) - _formalParamCount;
    
    _stack.push(Value(static_cast<uint32_t>(_stack.setLocalFrame(_formalParamCount, _actualParamCount, functionPtr->localSize())), Value::Type::PreviousFrame));
    _stack.push(Value(_pc, Value::Type::PreviousPC));
    _pc = 0;
    _stack.push(Value(_object, Value::Type::PreviousObject));
    _stack.push(Value(prevActualParamCount, Value::Type::PreviousParamCount));
    _object = function;

    _framePtr =_stack.framePtr();
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
        /* 0x14 */ OP(BINIOP) OP(EQ) OP(NE) OP(LT)
        /* 0x18 */ OP(LE) OP(GT) OP(GE) OP(BINIOP)
        /* 0x1C */ OP(BINIOP) OP(BINIOP) OP(ADD) OP(SUB)
        
        /* 0x20 */ OP(MUL)  OP(DIV)  OP(MOD)  OP(UNKNOWN)
        /* 0x24 */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x2c */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x30 */ OP(UMINUS)  OP(UNOT)  OP(UNEG)  OP(PREINC)
        /* 0x34 */ OP(PREDEC)  OP(POSTINC)  OP(POSTDEC)  OP(CALL)
        /* 0x38 */ OP(NEW)  OP(CALLPROP) OP(JMP)  OP(JT)
        /* 0x3c */ OP(JF) OP(END) OP(RET) OP(UNKNOWN)
    };
 
//static_assert (sizeof(dispatchTable) == (1 << 6) * sizeof(void*), "Dispatch table is wrong size");

static const uint16_t YieldCount = 10;
static const uint16_t GCCount = 1000;

    #undef DISPATCH
    #define DISPATCH { \
        if (_terminate) { \
            goto L_END; \
        } \
        if (--gcCounter == 0) { \
            gcCounter = GCCount; \
            _program->gc(this); \
            if (--yieldCounter == 0) { \
                return 0; \
            } \
        } \
        inst = _code[_pc++]; \
        goto *dispatchTable[static_cast<uint8_t>(inst.op())]; \
    }
    
    if (!_program) {
        return -1;
    }

    uint16_t gcCounter = GCCount;
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
    
    DISPATCH;
    
    L_UNKNOWN:
        assert(0);
        return -1;
    L_RET:
    L_RETX:
    L_END:
        if (_terminate || inst.op() == Op::END) {
            if (_terminate || _program == _functionPtr) {
                // We've hit the end of the program
                if (!_terminate) {
                    assert(_stack.validateFrame(0, _program->localSize()));
                }
                _program->gc(this);
                _stack.clear();
                return -1;
            }
            callReturnValue = CallReturnValue();
        }
        else {
            callReturnValue = CallReturnValue(CallReturnValue::Type::ReturnCount, inst.nparams());
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
        
        localsToPop = _functionPtr->localSize() + _localOffset;
        
        assert(_stack.top().type() == Value::Type::PreviousParamCount);
        _stack.pop(leftValue);
        _actualParamCount = leftValue.asPreviousParamCountValue();

        assert(_stack.top().type() == Value::Type::PreviousObject);
        _stack.pop(leftValue);
        _object = leftValue.asObjectIdValue();
        objectValue = _program->obj(_object);
        assert(objectValue->isFunction());
        _functionPtr = static_cast<Function*>(objectValue);
        _constantsPtr = _functionPtr->constantsPtr();
        
        assert(_stack.top().type() == Value::Type::PreviousPC);
        _stack.pop(leftValue);
        _pc = leftValue.asPreviousPCValue();
        
        assert(_stack.top().type() == Value::Type::PreviousFrame);
        _stack.pop(leftValue);
        _stack.restoreFrame(leftValue.asPreviousFrameValue(), localsToPop);
        _framePtr =_stack.framePtr();
        _formalParamCount = _functionPtr->formalParamCount();
        _localOffset = ((_formalParamCount < _actualParamCount) ? _actualParamCount : _formalParamCount) - _formalParamCount;
        
        updateCodePointer();
    
        _stack.push(returnedValue);
        DISPATCH;
    L_MOVE:
        setInFrame(inst.ra(), regOrConst(inst.rb()));
        DISPATCH;
    L_LOADREFK:
        setInFrame(inst.ra(), derefId(regOrConst(inst.rb()).asIdValue()));
        DISPATCH;
    L_LOADLITA:
    L_LOADLITO:
        if (inst.op() == Op::LOADLITA) {
            objectValue = new Array(_program);
        } else {
            objectValue = new MaterObject();
        }
        objectId = _program->addObject(objectValue, true);
        objectValue->setObjectId(objectId);
        setInFrame(inst.ra(), Value(objectId));
        DISPATCH;
    L_LOADPROP:
        objectValue = toObject(regOrConst(inst.rb()), "LOADPROP");
        if (!objectValue) {
            DISPATCH;
        }
        setInFrame(inst.ra(), objectValue->property(this, regOrConst(inst.rc()).toIdValue(this)));
        DISPATCH;
    L_LOADELT:
        objectValue = toObject(regOrConst(inst.rb()), "LOADELT");
        if (!objectValue) {
            DISPATCH;
        }
        setInFrame(inst.ra(), objectValue->element(this, regOrConst(inst.rc())));
        DISPATCH;
    L_STOPROP:
        objectValue = toObject(regOrConst(inst.ra()), "STOPROP");
        if (!objectValue) {
            DISPATCH;
        }
        if (!objectValue->setProperty(this, regOrConst(inst.rb()).toIdValue(this), regOrConst(inst.rc()), false)) {
            printError(ROMSTR("Property '%s' does not exist"), regOrConst(inst.rb()).toStringValue(this).c_str());
            checkTooManyErrors();
        }
            
        DISPATCH;
    L_STOELT:
        objectValue = toObject(regOrConst(inst.ra()), "STOPROP");
        if (!objectValue) {
            DISPATCH;
        }
        if (!objectValue->setElement(this, regOrConst(inst.rb()), regOrConst(inst.rc()), false)) {
            printError(ROMSTR("Element '%s' does not exist"), regOrConst(inst.rb()).toStringValue(this).c_str());
            checkTooManyErrors();
        }
        DISPATCH;
    L_APPENDELT:
        objectValue = toObject(regOrConst(inst.ra()), "APPENDELT");
        if (!objectValue) {
            DISPATCH;
        }
        objectValue->setElement(this, Value(), regOrConst(inst.rb()), true);
        DISPATCH;
    L_APPENDPROP:
        objectValue = toObject(regOrConst(inst.ra()), "APPENDPROP");
        if (!objectValue) {
            DISPATCH;
        }
        if (!objectValue->setProperty(this, regOrConst(inst.rb()).toIdValue(this), regOrConst(inst.rc()), true)) {
            printError(ROMSTR("Property '%s' already exists for APPENDPROP"), regOrConst(inst.rb()).toStringValue(this).c_str());
            DISPATCH;
        }
        DISPATCH;
    L_LOADTRUE:
        setInFrame(inst.ra(), Value(true));
        DISPATCH;
    L_LOADFALSE:
        setInFrame(inst.ra(), Value(false));
        DISPATCH;
    L_LOADNULL:
        setInFrame(inst.ra(), Value());
        DISPATCH;
    L_PUSH:
        _stack.push(regOrConst(inst.rn()));
        DISPATCH;
    L_POP:
        setInFrame(inst.ra(), _stack.top());
        _stack.pop();
        DISPATCH;
    L_BINIOP:
        leftIntValue = regOrConst(inst.rb()).toIntValue(this);
        rightIntValue = regOrConst(inst.rc()).toIntValue(this);
        switch(inst.op()) {
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
        setInFrame(inst.ra(), Value(leftIntValue));
        DISPATCH;
    L_EQ: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) == regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_NE: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) != regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_LT: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) < regOrConst(inst.rc()).toFloatValue(this)));  DISPATCH;
    L_LE: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) <= regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_GT: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) > regOrConst(inst.rc()).toFloatValue(this)));  DISPATCH;
    L_GE: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) >= regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_SUB: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) - regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_MUL: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) * regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_DIV: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) / regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_MOD: setInFrame(inst.ra(), Value(regOrConst(inst.rb()).toFloatValue(this) % regOrConst(inst.rc()).toFloatValue(this))); DISPATCH;
    L_ADD:
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());

        if (leftValue.isNumber() && rightValue.isNumber()) {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) + rightValue.toFloatValue(this)));
        } else {
            StringId stringId = _program->createString();
            String& s = _program->str(stringId);
            s = leftValue.toStringValue(this);
            s += rightValue.toStringValue(this);
            setInFrame(inst.ra(), Value(stringId));
        }
        DISPATCH;
    L_UMINUS: setInFrame(inst.ra(), -regOrConst(inst.rb()).toFloatValue(this)); DISPATCH;
    L_UNOT: setInFrame(inst.ra(), (regOrConst(inst.rb()).toIntValue(this) == 0) ? 1 : 0); DISPATCH;
    L_UNEG: setInFrame(inst.ra(), (regOrConst(inst.rb()).toIntValue(this) == 0) ? 0 : 1); DISPATCH;
    L_PREINC:
        setInFrame(inst.rb(), Value(regOrConst(inst.rb()).toIntValue(this) + 1));
        setInFrame(inst.ra(), regOrConst(inst.rb()));
        DISPATCH;
    L_PREDEC:
        setInFrame(inst.rb(), Value(regOrConst(inst.rb()).toIntValue(this) - 1));
        setInFrame(inst.ra(), regOrConst(inst.rb()));
        DISPATCH;
    L_POSTINC:
        setInFrame(inst.ra(), regOrConst(inst.rb()));
        setInFrame(inst.rb(), Value(regOrConst(inst.rb()).toIntValue(this) + 1));
        DISPATCH;
    L_POSTDEC:
        setInFrame(inst.ra(), regOrConst(inst.rb()));
        setInFrame(inst.rb(), Value(regOrConst(inst.rb()).toIntValue(this) - 1));
        DISPATCH;
    L_NEW:
    L_CALL:
    L_CALLPROP:
    
        objectValue = toObject(regOrConst(inst.rcall()), (inst.op() == Op::CALLPROP) ? "CALLPROP" : ((inst.op() == Op::CALL) ? "CALL" : "NEW"));
        if (!objectValue) {
            // Push a dummy value onto the stack for a return value
            _stack.push(Value());
            DISPATCH;
        }
        uintValue = inst.nparams();
        switch(inst.op()) {
            default: break;
            case Op::CALL:
                callReturnValue = objectValue->call(this, regOrConst(inst.rcall()), uintValue);
                break;
            case Op::NEW:
                callReturnValue = objectValue->construct(this, uintValue);
                break;
            case Op::CALLPROP:
                callReturnValue = objectValue->callProperty(this, regOrConst(inst.rthis()).asIdValue(), uintValue);
                break;
        }

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
        intValue = inst.sn();
        if (inst.op() != Op::JMP) {
            boolValue = regOrConst(inst.rn()).toBoolValue(this);
            if (inst.op() == Op::JT) {
                boolValue = !boolValue;
            }
            if (boolValue) {
                DISPATCH;
            }
        }
        _pc += intValue - 1;
        DISPATCH;
}
