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
        system()->printf(ROMSTR("\n\nToo many runtime errors, (%d) exiting...\n"), _nerrors);
        _terminate = true;
        return false;
    }
    return true;
}

bool ExecutionUnit::printError(const char* format, ...) const
{
    va_list args;
    va_start(args, format);
    Error::vprintError(system(), Error::Code::RuntimeError, _lineno, format, args);
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

void ExecutionUnit::stoIdRef(Atom atom, const Value& value)
{
    if (!atom) {
        printError(ROMSTR("Destination in STOREFK must be an Atom"));
        return;
    }
    
    // If property exists in this, store it there
    if (_this) {
        Value oldValue = _this->property(this, atom);
        if (oldValue) {
            if (!_this->setProperty(this, atom, value, Object::SetPropertyType::AddIfNeeded)) {
                printError(ROMSTR("'%s' property of this object cannot be set"), _program->stringFromAtom(atom).c_str());
            }
            return;
        }
    }
    
    // FIXME: Do we ever want to set a property in program or global object?
    printError(ROMSTR("'%s' property does not exist or cannot be set"), _program->stringFromAtom(atom).c_str());
    return;
}

Value ExecutionUnit::derefId(Atom atom)
{
    if (!atom) {
        printError(ROMSTR("Value in LOADREFK must be an Atom"));
        return Value();
    }
    
    if (atom == ATOM(__this)) {
        return Value(_thisId);
    }

    // Look in this then program then global
    Value value;
    if (_this) {
        value = _this->property(this, atom);
        if (value) {
            return value;
        }
    }

    value = _program->property(this, atom);
    if (value) {
        return value;
    }
    
    value = _program->global()->property(this, atom);
    if (value) {
        return value;
    }
    
    printError(ROMSTR("'%s' property does not exist in global scope"), _program->stringFromAtom(atom).c_str());
    return Value();
}

void ExecutionUnit::startExecution(Program* program)
{
    if (!program) {
        _terminate = true;
        _function = nullptr;
        _this = nullptr;
        _constants = nullptr;
        _stack.clear();
        return;
    }
    _terminate = false;
    _nerrors = 0;
    _pc = 0;
    _program = program;
    _function =  _program;
    _constants = _function->constantsPtr();
    
    _thisId = program->objectId();
    _this = program;
    
    _stack.clear();
    _stack.setLocalFrame(0, 0, _function->localSize());
    _framePtr =_stack.framePtr();
    _formalParamCount = 0;
    _actualParamCount = 0;
    _localOffset = 0;
    _executingEvent = false;
}

void ExecutionUnit::fireEvent(const Value& func, const Value& thisValue, const Value* args, int32_t nargs)
{
    TaskManager::lock();
    
    _eventQueue.push_back(func);
    _eventQueue.push_back(thisValue);
    _eventQueue.push_back(Value(nargs));
    for (int i = 0; i < nargs; i++) {
        _eventQueue.push_back(args[i]);
    }

    TaskManager::unlock();
}

void ExecutionUnit::runNextEvent()
{
    // Each event is at least 3 entries long. First is the function, followed by the this pointer
    // followed by the number of args. That number of args then follows    
    Value func;
    Value thisValue;
    int32_t nargs = 0;
    bool haveEvent = false;
    
    TaskManager::lock();

    if (!_eventQueue.empty()) {
        assert(_eventQueue.size() >= 3);
        
        haveEvent = true;
        _executingEvent = true;
        func = _eventQueue[0];
        thisValue = _eventQueue[1];
        nargs = _eventQueue[2].asIntValue();
        assert(_eventQueue.size() >= 3 + nargs);
        
        for (int32_t i = 0; i < nargs; ++i) {
            _stack.push(_eventQueue[3 + i]);
        }
        
        _eventQueue.erase(_eventQueue.begin(), _eventQueue.begin() + 3 + nargs);
    }

    TaskManager::unlock();
        
    if (haveEvent) {
        startFunction(func.asFunction(), thisValue.asObjectIdValue(), nargs, false);
    }
}

void ExecutionUnit::startFunction(Function* function, ObjectId thisObject, uint32_t nparams, bool inScope)
{
    assert(_program);
    assert(function);
    
    Function* prevFunction = _function;
    _function =  function;
    assert(_function->code()->size());

    _constants = _function->constantsPtr();
    _formalParamCount = _function->formalParamCount();
    uint32_t prevActualParamCount = _actualParamCount;
    _actualParamCount = nparams;
    _localOffset = ((_formalParamCount < _actualParamCount) ? _actualParamCount : _formalParamCount) - _formalParamCount;
    
    ObjectId prevThisId = _thisId;
    _thisId = thisObject;
    _this = Global::obj(_thisId);
    
    uint32_t prevFrame = _stack.setLocalFrame(_formalParamCount, _actualParamCount, _function->localSize());
    
    _callRecords.push_back({ _pc, prevFrame, prevFunction, prevThisId, prevActualParamCount, _inScope });
    _inScope = inScope;
    
    _pc = 0;    
    _framePtr =_stack.framePtr();
}

static inline bool valuesAreInt(const Value& a, const Value& b)
{
    return a.isInteger() && b.isInteger();
}

CallReturnValue ExecutionUnit::continueExecution()
{
    #undef OP
    #define OP(op) &&L_ ## op,
    
    static const void* RODATA_ATTR dispatchTable[] {
        /* 0x00 */ OP(MOVE) OP(LOADREFK) OP(STOREFK) OP(LOADLITA)
        /* 0x04 */ OP(LOADLITO) OP(LOADPROP) OP(LOADELT) OP(STOPROP)
        /* 0x08 */ OP(STOELT) OP(APPENDELT) OP(APPENDPROP) OP(LOADTRUE)
        /* 0x0C */ OP(LOADFALSE) OP(LOADNULL) OP(PUSH) OP(POP)

        /* 0x10 */ OP(BINIOP) OP(BINIOP) OP(BINIOP) OP(BINIOP)
        /* 0x14 */ OP(BINIOP) OP(EQ) OP(NE) OP(LT)
        /* 0x18 */ OP(LE) OP(GT) OP(GE) OP(BINIOP)
        /* 0x1C */ OP(BINIOP) OP(BINIOP) OP(ADD) OP(SUB)
        
        /* 0x20 */ OP(MUL)  OP(DIV)  OP(MOD)  OP(LINENO)
        /* 0x24 */ OP(LOADTHIS)  OP(LOADUP)  OP(STOREUP)  OP(UNKNOWN)
        /* 0x28 */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x2c */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x30 */ OP(UMINUS)  OP(UNOT)  OP(UNEG)  OP(PREINC)
        /* 0x34 */ OP(PREDEC)  OP(POSTINC)  OP(POSTDEC)  OP(CALL)
        /* 0x38 */ OP(NEW)  OP(CALLPROP) OP(JMP)  OP(JT)
        /* 0x3c */ OP(JF) OP(END) OP(RET) OP(UNKNOWN)
    };
 
static_assert (sizeof(dispatchTable) == (1 << 6) * sizeof(void*), "Dispatch table is wrong size");

static const uint16_t YieldCount = 10;
static const uint16_t GCCount = 1000;

    #undef DISPATCH
    #define DISPATCH { \
        if (_terminate) { \
            goto L_END; \
        } \
        if (--gcCounter == 0) { \
            gcCounter = GCCount; \
            Global::gc(this); \
            if (--yieldCounter == 0) { \
                return CallReturnValue(CallReturnValue::Type::Continue); \
            } \
        } \
        inst = _code[_pc++]; \
        goto *dispatchTable[static_cast<uint8_t>(inst.op())]; \
    }
    
    if (!_program) {
        return CallReturnValue(CallReturnValue::Type::Finished);
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
    uint32_t localsToPop;
    ObjectId prevThisId;

    Instruction inst;
    
    DISPATCH;
    
    L_LINENO:
        _lineno = inst.un();
        DISPATCH;
        
    L_UNKNOWN:
        assert(0);
        return CallReturnValue(CallReturnValue::Type::Finished);
    L_RET:
    L_RETX:
    L_END:
        if (_terminate) {
            _stack.clear();
            Global::gc(this);
            return CallReturnValue(CallReturnValue::Type::Terminated);
        }
            
        if (inst.op() == Op::END) {
            if (_program == _function) {
                // We've hit the end of the program
                // If we were executing an event there will be a return value on the stack
                if (_executingEvent) {
                    _stack.pop();
                    _executingEvent = false;
                }
                
                if (!_stack.validateFrame(0, _program->localSize())) {
                    printError(ROMSTR("internal error. On exit stack has %d elements, should have %d"), _stack.size(), _program->localSize());
                    _terminate = true;
                    _stack.clear();
                    Global::gc(this);
                    return CallReturnValue(CallReturnValue::Type::Terminated);
                }
                
                Global::gc(this);
                
                // Backup the PC to point at the END instruction, so when we return from events
                // we'll hit the program end again
                _pc--;
                assert(_code[_pc].op() == Op::END);
                
                // FIXME: For now we always wait for events. But we need to add logic to only do that if we have
                // something listening. For instance if we have an active TCP socket or an interval timer
                runNextEvent();
                return CallReturnValue(CallReturnValue::Type::WaitForEvent);
            }
            callReturnValue = CallReturnValue();
        }
        else {
            callReturnValue = CallReturnValue(CallReturnValue::Type::ReturnCount, inst.nparams());
        }
        
        returnedValue = (callReturnValue.isReturnCount() && callReturnValue.returnCount() > 0) ? _stack.top(1 - callReturnValue.returnCount()) : Value();
        _stack.pop(callReturnValue.returnCount());
        
        localsToPop = _function->localSize() + _localOffset;
        
        {
            // Get the call record entries from the call stack and pop it.
            assert(!_callRecords.empty());
            const CallRecord& callRecord = _callRecords.back();

            prevThisId = _thisId;
            
            _actualParamCount = callRecord._paramCount;
            _thisId = callRecord._thisId;
            assert(_thisId);
            _this = Global::obj(_thisId);
            _stack.restoreFrame(callRecord._frame, localsToPop);
            _framePtr =_stack.framePtr();
            
            _pc = callRecord._pc;
            _function = callRecord._func;
            _inScope = callRecord._inScope;
            
            _callRecords.resize(_callRecords.size() - 1);
        }
        
        assert(_function->code()->size());
        _constants = _function->constantsPtr();

        _formalParamCount = _function->formalParamCount();
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
    L_STOREFK:
        stoIdRef(regOrConst(inst.rb()).asIdValue(), regOrConst(inst.rc()));
        DISPATCH;
    L_LOADUP:
        rightValue = _function->loadUpValue(this, inst.rb());
        setInFrame(inst.ra(), rightValue);
        DISPATCH;
    L_STOREUP:
        _function->storeUpValue(this, inst.ra(), regOrConst(inst.rb()));
        DISPATCH;
    L_LOADLITA:
    L_LOADLITO:
        if (inst.op() == Op::LOADLITA) {
            objectValue = new Array(_program);
        } else {
            objectValue = new MaterObject();
        }
        objectId = Global::addObject(objectValue, true);
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
        if (!objectValue->setProperty(this, regOrConst(inst.rb()).toIdValue(this), regOrConst(inst.rc()), Object::SetPropertyType::AddIfNeeded)) {
            printError(ROMSTR("Property '%s' does not exist"), regOrConst(inst.rb()).toStringValue(this).c_str());
            checkTooManyErrors();
        }
            
        DISPATCH;
    L_STOELT:
        objectValue = toObject(regOrConst(inst.ra()), "STOELT");
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
        if (!objectValue->setProperty(this, regOrConst(inst.rb()).toIdValue(this), regOrConst(inst.rc()), Object::SetPropertyType::AlwaysAdd)) {
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
    L_LOADTHIS:
        setInFrame(inst.ra(), Value(_thisId));
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
    L_EQ: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() == rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) == rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_NE: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() != rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) != rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_LT: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() < rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) < rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_LE: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() <= rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) <= rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_GT: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() > rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) > rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_GE: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() >= rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) >= rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_SUB: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() - rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) - rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_MUL: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() * rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) * rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_DIV: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() / rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) / rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_MOD: 
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() % rightValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) % rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_ADD:
        leftValue = regOrConst(inst.rb());
        rightValue = regOrConst(inst.rc());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(inst.ra(), Value(leftValue.asIntValue() + rightValue.asIntValue()));
        } else if (leftValue.isNumber() && rightValue.isNumber()) {
            setInFrame(inst.ra(), Value(leftValue.toFloatValue(this) + rightValue.toFloatValue(this)));
        } else {
            StringId stringId = Global::createString(leftValue.toStringValue(this) + rightValue.toStringValue(this));
            setInFrame(inst.ra(), Value(stringId));
        }
        DISPATCH;
    L_UMINUS:
        leftValue = regOrConst(inst.rb());
        if (leftValue.isInteger()) {
            setInFrame(inst.ra(), -leftValue.asIntValue());
        } else {
            setInFrame(inst.ra(), -leftValue.toFloatValue(this));
        }
        DISPATCH;
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
        uintValue = inst.nparams();
        if (objectValue) {
            switch(inst.op()) {
                default: break;
                case Op::CALL: {
                    bool inScope = isConstant(inst.rcall());
                    leftValue = regOrConst(inst.rthis());
                    if (!leftValue) {
                        leftValue = Value(_thisId);
                    }
                    callReturnValue = objectValue->call(this, leftValue, uintValue, false, inScope);
                    break;
                }
                case Op::NEW:
                    callReturnValue = objectValue->call(this, Value(), uintValue, true, false);
                    break;
                case Op::CALLPROP:
                    callReturnValue = objectValue->callProperty(this, regOrConst(inst.rthis()).asIdValue(), uintValue);
                    break;
            }
        } else {
            // Simulate a return count of 0. No need to flag this as an error. We already showed an error above
            callReturnValue = CallReturnValue();
        }
        
        if (callReturnValue.isError()) {
            printError(ROMSTR("%sing function"), (inst.op() == Op::NEW) ? "construct" : "call");
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
            return callReturnValue;
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
