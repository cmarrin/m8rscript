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

#include "Closure.h"
#include "Float.h"
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

bool ExecutionUnit::printError(CallReturnValue::Error error, const char* name) const
{
    const char* errorString = ROMSTR("*UNKNOWN*");
    switch(error) {
        case CallReturnValue::Error::Ok: return true;
        case CallReturnValue::Error::WrongNumberOfParams: errorString = ROMSTR("wrong number of params"); break;
        case CallReturnValue::Error::ConstructorOnly: errorString = ROMSTR("only valid for new"); break;
        case CallReturnValue::Error::Unimplemented: errorString = ROMSTR("unimplemented function"); break;
        case CallReturnValue::Error::OutOfRange: errorString = ROMSTR("param out of range"); break;
        case CallReturnValue::Error::MissingThis: errorString = ROMSTR("Missing this value"); break;
        case CallReturnValue::Error::InternalError: errorString = ROMSTR("internal error"); break;
        case CallReturnValue::Error::PropertyDoesNotExist: errorString = ROMSTR("'%s' property does not exist"); break;
        case CallReturnValue::Error::BadFormatString: errorString = ROMSTR("bad format string"); break;
        case CallReturnValue::Error::UnknownFormatSpecifier: errorString = ROMSTR("unknown format specifier"); break;
        case CallReturnValue::Error::CannotConvertStringToNumber: errorString = ROMSTR("string cannot be converted"); break;
        case CallReturnValue::Error::CannotCreateArgumentsArray: errorString = ROMSTR("cannot create arguments array"); break;
        case CallReturnValue::Error::CannotCall: errorString = ROMSTR("cannot call value of this type"); break;
        case CallReturnValue::Error::InvalidArgumentValue: errorString = ROMSTR("invalid argument value"); break;
    }
    
    return printError(errorString, name);
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
            if (!_this->setProperty(this, atom, value, Value::SetPropertyType::AddIfNeeded)) {
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
    
    if (atom == ATOM(this, __this)) {
        return Value(_this);
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

void ExecutionUnit::closeUpValues(uint32_t frame)
{
    UpValue* prev = nullptr;
    for (UpValue* upValue = _openUpValues; upValue; upValue = upValue->next()) {
        if (upValue->closeIfNeeded(this, frame)) {
            if (prev) {
                prev->setNext(upValue->next());
            } else {
                _openUpValues = upValue->next();
            }
        } else {
            prev = upValue;
        }
    }
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
    _callRecords.clear();
    _stack.clear();

    _pc = 0;
    _program = program;
    _function =  _program;
    _this = program;
    _constants = _function->constants() ? &(_function->constants()->at(0)) : nullptr;
    _stack.setLocalFrame(0, 0, _function->localSize());
    _framePtr =_stack.framePtr();

    _localOffset = 0;
    _formalParamCount = 0;
    _actualParamCount = 0;

    _nerrors = 0;
    _terminate = false;
    
    _eventQueue.clear();
    _executingEvent = false;
    _numEventListeners = 0;
    _lineno = 0;

    _openUpValues = nullptr;
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

CallReturnValue ExecutionUnit::runNextEvent()
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
        CallReturnValue callReturnValue = func.call(this, Value(), nargs, false);
                
        // Callbacks don't return a value. Ignore it, but pop the stack
        if (callReturnValue.isReturnCount()) {
            if (callReturnValue.returnCount() > 0) {
                _stack.pop(callReturnValue.returnCount());
            }
            _stack.pop(nargs);
        }
        return callReturnValue;
    }
    
    return CallReturnValue(CallReturnValue::Type::WaitForEvent);
}

uint32_t ExecutionUnit::upValueStackIndex(uint32_t index, uint16_t frame) const
{
    assert(frame > 0);
    frame--;
    assert(frame <= _callRecords.size());
    if (frame == 0) {
        return _stack.frame() + index;
    }
    uint32_t stackIndex = _callRecords[_callRecords.size() - frame]._frame + index;
    assert(stackIndex < _stack.size());
    return stackIndex;
}

UpValue* ExecutionUnit::newUpValue(uint32_t stackIndex)
{
    for (auto next = _openUpValues; next; next = next->next()) {
        assert(!next->closed());
        if (next->stackIndex() == stackIndex) {
            return next;
        }
    }
    UpValue* upValue = new UpValue(stackIndex);
    upValue->setNext(_openUpValues);
    _openUpValues = upValue;
    return upValue;
}

void ExecutionUnit::startFunction(Object* function, Object* thisObject, uint32_t nparams, bool inScope)
{
    assert(_program);
    assert(function);
    
    Object* prevFunction = _function;
    _function =  function;
    assert(_function->code() && _function->code()->size());

    _constants = _function->constants() ? &(_function->constants()->at(0)) : nullptr;
    _formalParamCount = _function->formalParamCount();
    uint32_t prevActualParamCount = _actualParamCount;
    _actualParamCount = nparams;
    _localOffset = ((_formalParamCount < _actualParamCount) ? _actualParamCount : _formalParamCount) - _formalParamCount;
    
    Object* prevThis = _this;
    _this = thisObject;
    if (!_this) {
        _this = _program;
    }
    
    uint32_t prevFrame = _stack.setLocalFrame(_formalParamCount, _actualParamCount, _function->localSize());
    
    _callRecords.push_back({ _pc, prevFrame, prevFunction, prevThis, prevActualParamCount, _lineno });
    
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
        /* 0x24 */ OP(LOADTHIS)  OP(LOADUP)  OP(STOREUP)  OP(CLOSURE)
        /* 0x28 */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x2c */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x30 */ OP(UMINUS)  OP(UNOT)  OP(UNEG)  OP(PREINC)
        /* 0x34 */ OP(PREDEC)  OP(POSTINC)  OP(POSTDEC)  OP(CALL)
        /* 0x38 */ OP(NEW)  OP(CALLPROP) OP(JMP)  OP(JT)
        /* 0x3c */ OP(JF) OP(END) OP(RET) OP(UNKNOWN)
    };
 
static_assert (sizeof(dispatchTable) == (1 << 6) * sizeof(void*), "Dispatch table is wrong size");

    #undef DISPATCH
    #define DISPATCH { \
        ++checkCounter; \
        if ((checkCounter & 0xf) == 0) { \
            if (_terminate) { \
                goto L_END; \
            } \
            if ((checkCounter & 0xff) == 0) { \
                Object::gc(this, false); \
            } \
            if (checkCounter == 0) { \
                return CallReturnValue(CallReturnValue::Type::Continue); \
            } \
        } \
        inst = _code[_pc++]; \
        goto *dispatchTable[static_cast<uint8_t>(inst.op())]; \
    }
    
    if (!_program) {
        return CallReturnValue(CallReturnValue::Type::Finished);
    }

    Object::gc(this, false);

    uint16_t checkCounter = 0;
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
    Value returnedValue;
    CallReturnValue callReturnValue;
    uint32_t localsToPop;
    Object* prevThis;

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
            Object::gc(this, true);
            return CallReturnValue(CallReturnValue::Type::Terminated);
        }
            
        if (inst.op() == Op::END) {
            if (_program == _function) {
                // We've hit the end of the program
                
                if (!_stack.validateFrame(0, _program->localSize())) {
                    printError(ROMSTR("internal error. On exit stack has %d elements, should have %d"), _stack.size(), _program->localSize());
                    _terminate = true;
                    _stack.clear();
                    Object::gc(this, true);
                    return CallReturnValue(CallReturnValue::Type::Terminated);
                }
                
                Object::gc(this, true);
                
                // Backup the PC to point at the END instruction, so when we return from events
                // we'll hit the program end again
                _pc--;
                assert(_code[_pc].op() == Op::END);
                
                callReturnValue = runNextEvent();

                if (callReturnValue.isError()) {
                    printError(callReturnValue.error());
                    return CallReturnValue(CallReturnValue::Type::Finished);
                }

                // If the callReturnValue is FunctionStart it means we've called a Function and it just
                // setup the EU to execute it. In that case just continue
                if (callReturnValue.isFunctionStart()) {
                    updateCodePointer();
                    DISPATCH;
                }
                return CallReturnValue(_numEventListeners ? CallReturnValue::Type::WaitForEvent : CallReturnValue::Type::Finished);
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
            
            // Close any upValues that need it
            closeUpValues(callRecord._frame);

            prevThis = _this;
            
            _actualParamCount = callRecord._paramCount;
            _this = callRecord._thisObj;
            assert(_this);
            _stack.restoreFrame(callRecord._frame, localsToPop);
            _framePtr =_stack.framePtr();
            
            _pc = callRecord._pc;
            _function = callRecord._func;
            
            _lineno = callRecord._lineno;
            
            _callRecords.pop_back();
        }
        
        assert(_function->code()->size());
        _constants = _function->constants() ? &(_function->constants()->at(0)) : nullptr;

        _formalParamCount = _function->formalParamCount();
        _localOffset = ((_formalParamCount < _actualParamCount) ? _actualParamCount : _formalParamCount) - _formalParamCount;

        updateCodePointer();

        // If we were executing an event don't push the return value
        if (_executingEvent) {
            _executingEvent = false;
        } else {
            _stack.push(returnedValue);
        }
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
    L_LOADPROP:
        leftValue = regOrConst(inst.rb());
        objectValue = leftValue.asObject();
        leftValue = leftValue.property(this, regOrConst(inst.rc()).toIdValue(this));
        if (!objectValue) {
            objectError("LOADPROP");
        } else if (!leftValue) {
            printError(ROMSTR("Property '%s' does not exist"), regOrConst(inst.rc()).toStringValue(this).c_str());
        } else {
            setInFrame(inst.ra(), leftValue);
        }
        DISPATCH;
    L_STOPROP:
        if (!reg(inst.ra()).setProperty(this, regOrConst(inst.rb()).toIdValue(this), regOrConst(inst.rc()), Value::SetPropertyType::NeverAdd)) {
            printError(ROMSTR("Property '%s' does not exist"), regOrConst(inst.rb()).toStringValue(this).c_str());
        }
        DISPATCH;
    L_LOADELT:
        leftValue = regOrConst(inst.rb()).element(this, regOrConst(inst.rc()));
        if (!leftValue) {
            objectError("LOADELT");
        } else {
            setInFrame(inst.ra(), leftValue);
        }
        DISPATCH;
    L_STOELT:
        if (!reg(inst.ra()).setElement(this, regOrConst(inst.rb()), regOrConst(inst.rc()), false)) {
            printError(ROMSTR("Element '%s' does not exist"), regOrConst(inst.rb()).toStringValue(this).c_str());
        }
        DISPATCH;
    L_LOADUP:
        if (!_function->loadUpValue(this, inst.rb(), rightValue)) {
            printError(ROMSTR("unable to load upValue"));
        } else {
            setInFrame(inst.ra(), rightValue);
        }
        DISPATCH;
    L_STOREUP:
        if (!_function->storeUpValue(this, inst.ra(), regOrConst(inst.rb()))) {
            printError(ROMSTR("unable to store upValue"));
        }
        DISPATCH;
    L_LOADLITA:
    L_LOADLITO:
        if (inst.op() == Op::LOADLITA) {
            objectValue = new MaterObject(true);
        } else {
            objectValue = new MaterObject();
        }
        setInFrame(inst.ra(), Value(objectValue));
        DISPATCH;
    L_APPENDPROP:
        if (!reg(inst.ra()).setProperty(this, regOrConst(inst.rb()).toIdValue(this), regOrConst(inst.rc()), Value::SetPropertyType::AlwaysAdd)) {
            printError(ROMSTR("Property '%s' already exists for APPENDPROP"), regOrConst(inst.rb()).toStringValue(this).c_str());
        }
        DISPATCH;
    L_APPENDELT:
        if (!reg(inst.ra()).setElement(this, Value(), regOrConst(inst.rb()), true)) {
            objectError("APPENDELT");
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
        setInFrame(inst.ra(), Value(_this));
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
            String* string = Object::createString(leftValue.toStringValue(this) + rightValue.toStringValue(this));
            setInFrame(inst.ra(), Value(string));
        }
        DISPATCH;
    L_UMINUS:
        leftValue = regOrConst(inst.rb());
        if (leftValue.isInteger()) {
            setInFrame(inst.ra(), Value(-leftValue.asIntValue()));
        } else {
            setInFrame(inst.ra(), Value(-leftValue.toFloatValue(this)));
        }
        DISPATCH;
    L_UNEG:
        setInFrame(inst.ra(), Value((regOrConst(inst.rb()).toIntValue(this) == 0) ? 1 : 0));
        DISPATCH;
    L_UNOT:
        setInFrame(inst.ra(), Value(~(regOrConst(inst.rb()).toIntValue(this))));
        DISPATCH;
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
    L_CLOSURE: {
        Closure* closure = new Closure(this, regOrConst(inst.rb()), _this ? Value(_this) : Value());
        setInFrame(inst.ra(), Value(closure));
        DISPATCH;
    }
    L_NEW:
    L_CALL:
    L_CALLPROP: {
        leftValue = regOrConst(inst.rcall());
        uintValue = inst.nparams();
        Atom name;

        switch(inst.op()) {
            default: break;
            case Op::CALL: {
                rightValue = regOrConst(inst.rthis());
                if (!rightValue) {
                    rightValue = Value(_this);
                }
                callReturnValue = leftValue.call(this, rightValue, uintValue, false);
                break;
            }
            case Op::NEW:
                callReturnValue = leftValue.call(this, Value(), uintValue, true);
                break;
            case Op::CALLPROP:
                name = regOrConst(inst.rthis()).asIdValue();
                callReturnValue = leftValue.callProperty(this, name, uintValue);
                break;
        }
        
        if (callReturnValue.isError()) {
            printError(callReturnValue.error(), _program->stringFromAtom(name).c_str());
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
    }
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
