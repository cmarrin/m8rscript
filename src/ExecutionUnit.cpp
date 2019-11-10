/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "ExecutionUnit.h"

#include "Closure.h"
#include "Float.h"
#include "MStream.h"
#include "Parser.h"
#include "SystemInterface.h"
#include "SystemTime.h"

using namespace m8r;

static const Duration EvalDurationMax = 2_sec;

void ExecutionUnit::tooManyErrors() const
{
    system()->printf(ROMSTR("\n\nToo many runtime errors, (%d) exiting...\n"), _nerrors);
}

void ExecutionUnit::printError(const char* format, ...) const
{
    va_list args;
    va_start(args, format);
    system()->printf(ROMSTR("***** "));

    Error::vprintError(Error::Code::RuntimeError, _lineno, format, args);
    ++_nerrors;
}

void ExecutionUnit::printError(CallReturnValue::Error error) const
{
    const char* errorString = ROMSTR("*UNKNOWN*");
    switch(error) {
        case CallReturnValue::Error::Ok: return;
        case CallReturnValue::Error::WrongNumberOfParams: errorString = ROMSTR("wrong number of params"); break;
        case CallReturnValue::Error::ConstructorOnly: errorString = ROMSTR("only valid for new"); break;
        case CallReturnValue::Error::Unimplemented: errorString = ROMSTR("unimplemented function"); break;
        case CallReturnValue::Error::OutOfRange: errorString = ROMSTR("param out of range"); break;
        case CallReturnValue::Error::MissingThis: errorString = ROMSTR("Missing this value"); break;
        case CallReturnValue::Error::InternalError: errorString = ROMSTR("internal error"); break;
        case CallReturnValue::Error::PropertyDoesNotExist: errorString = ROMSTR("property does not exist"); break;
        case CallReturnValue::Error::BadFormatString: errorString = ROMSTR("bad format string"); break;
        case CallReturnValue::Error::UnknownFormatSpecifier: errorString = ROMSTR("unknown format specifier"); break;
        case CallReturnValue::Error::CannotConvertStringToNumber: errorString = ROMSTR("string cannot be converted"); break;
        case CallReturnValue::Error::CannotCreateArgumentsArray: errorString = ROMSTR("cannot create arguments array"); break;
        case CallReturnValue::Error::CannotCall: errorString = ROMSTR("cannot call value of this type"); break;
        case CallReturnValue::Error::InvalidArgumentValue: errorString = ROMSTR("invalid argument value"); break;
        case CallReturnValue::Error::SyntaxErrors: errorString = ROMSTR("syntax errors"); break;
        case CallReturnValue::Error::ImportTimeout: errorString = ROMSTR("import() timeout"); break;
        case CallReturnValue::Error::DelayNotAllowedInImport: errorString = ROMSTR("delay not allowed in import()"); break;
        case CallReturnValue::Error::EventNotAllowedInImport: errorString = ROMSTR("event not allowed in import()"); break;
        case CallReturnValue::Error::Error: errorString = ROMSTR("error"); break;
    }
    
    printError(errorString);
}

void ExecutionUnit::gcMark()
{
    if (!_program) {
        return;
    }
    
    for (auto entry : _stack) {
        entry.gcMark();
    }
    
    _program->gcMark();

    for (auto it : _eventQueue) {
        it.gcMark();
    }
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

    // See if it's in global
    Value oldValue = _program->global()->property(this, atom);
    if (oldValue) {
        if (!_program->global()->setProperty(this, atom, value, Value::SetPropertyType::AddIfNeeded)) {
            printError(ROMSTR("'%s' property of this object cannot be set"), _program->stringFromAtom(atom).c_str());
        }
        return;
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
    
    if (atom == Atom(SA::__this)) {
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

void ExecutionUnit::startExecution(Mad<Program> program)
{
    if (!program) {
        _terminate = true;
        _function.reset();
        _this.reset();
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
    system()->lock();
    
    _eventQueue.push_back(func);
    _eventQueue.push_back(thisValue);
    _eventQueue.push_back(Value(nargs));
    for (int i = 0; i < nargs; i++) {
        _eventQueue.push_back(args[i]);
    }

    system()->unlock();
}

void ExecutionUnit::receivedData(const String& data, Telnet::Action action)
{
    // Get the consoleListener from Global and use that to fire an event
    Value listener = program()->global()->property(this, Atom(SA::consoleListener));
    if (listener && !listener.isNull()) {
        Value args[2];
        args[0] = Value(Object::createString(data));

        // Action is an enum, but it is always a 1-4 character string encoded as a uint32_t.
        // Convert it to a StringLiteral
        char a[5];
        uint32_t actionInt = static_cast<uint32_t>(action);
        a[0] = actionInt >> 24;
        a[1] = actionInt >> 16;
        a[2] = actionInt >> 8;
        a[3] = actionInt;
        a[4] = '\0';
        args[1] = a[0] ? Value(_program->stringLiteralFromString(a)) : Value();
        
        fireEvent(listener, Value(), args, 2);
    }
}

// This function will only ever return MSDelay, Yield, WaitForEvent and Error.
// everything else is handled
CallReturnValue ExecutionUnit::runNextEvent()
{
    // Each event is at least 3 entries long. First is the function, followed by the this pointer
    // followed by the number of args. That number of args then follows    
    Value func;
    Value thisValue;
    int32_t nargs = 0;
    bool haveEvent = false;
    
    system()->lock();

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

    system()->unlock();
    
    if (haveEvent) {
        CallReturnValue callReturnValue = func.call(this, Value(), nargs, false);
                
        // Callbacks don't return a value. Ignore it, but pop the stack
        if (callReturnValue.isReturnCount()) {
            if (callReturnValue.returnCount() > 0) {
                _stack.pop(callReturnValue.returnCount());
            }
            _stack.pop(nargs);
            callReturnValue = CallReturnValue(CallReturnValue::Type::Yield);
        } else if (callReturnValue.isFunctionStart()) {
            updateCodePointer();
            callReturnValue = CallReturnValue(CallReturnValue::Type::Yield);
        } else if (callReturnValue.isFinished() || callReturnValue.isTerminated()) {
            callReturnValue = CallReturnValue(CallReturnValue::Error::InternalError);
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

void ExecutionUnit::startFunction(Mad<Object> function, Mad<Object> thisObject, uint32_t nparams, bool inScope)
{
    assert(_program);
    assert(function);
    
    Mad<Object> prevFunction = _function;
    _function =  function;
    assert(_function->code() && _function->code()->size());

    _constants = _function->constants() ? &(_function->constants()->at(0)) : nullptr;
    _formalParamCount = _function->formalParamCount();
    uint32_t prevActualParamCount = _actualParamCount;
    _actualParamCount = nparams;
    _localOffset = ((_formalParamCount < _actualParamCount) ? _actualParamCount : _formalParamCount) - _formalParamCount;
    
    Mad<Object> prevThis = _this;
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

static inline int compareFloat(Float a, Float b)
{
    Float result = a - b;
    return (result < Float(0)) ? -1 : ((result > Float(0)) ? 1 : 0);
}

int ExecutionUnit::compareValues(const Value& a, const Value& b)
{
    if ((a.isNull() || a.isNone()) && (b.isNull() || b.isNone())) {
        return true;
    }
    
    if (valuesAreInt(a, b)) {
        return a.asIntValue() - b.asIntValue();
    }
    
    // Optimization for equal string literals
    if (a.isStringLiteral() && b.isStringLiteral()) {
        if (a.asStringLiteralValue() == b.asStringLiteralValue()) {
            return 0;
        }
        // Otherwise just fall through and test as strings
    }
    
    if (a.isString() && b.isString()) {
        return String::compare(a.toStringValue(this), b.toStringValue(this));
    }
    
    if ((a.isNumber() || a.isString()) && (b.isNumber() || b.isString())) {
        return compareFloat(a.toFloatValue(this), b.toFloatValue(this));
    }
    
    // TODO: Handle converting Object to primitive value and comparing that.
    // A primitive value is either a Number or a String
    return -1;
}

CallReturnValue ExecutionUnit::import(const Stream& stream, Value thisValue)
{
    Parser parser(_program);
    ErrorList syntaxErrors;
    
    // Contents of import are placed inside the parent Function and then they will
    // be extracted into an Object
    Mad<Function> parent = Mad<Function>::create();
    
    Mad<Function> function = parser.parse(stream, Parser::Debug::Full, parent);
    if (parser.nerrors()) {
        syntaxErrors.swap(parser.syntaxErrors());
        
        // TODO: Do something with syntaxErrors
        return CallReturnValue(CallReturnValue::Error::SyntaxErrors);
    }
    
    // Get all the contents into a new object
    Mad<Object> obj = Mad<MaterObject>::create();
    
    // Get any constant functions
    for (auto it : *(function->constants())) {
        Mad<Function> func = it.asFunction();
        if (func && func->name()) {
            obj->setProperty(this, func->name(), Value(func), Value::SetPropertyType::AlwaysAdd);
        }
    }

    stack().push(Value(obj));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
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

        /* 0x10 */ OP(LOR) OP(LAND) OP(BINIOP) OP(BINIOP)
        /* 0x14 */ OP(BINIOP) OP(EQ) OP(NE) OP(LT)
        /* 0x18 */ OP(LE) OP(GT) OP(GE) OP(BINIOP)
        /* 0x1C */ OP(BINIOP) OP(BINIOP) OP(ADD) OP(SUB)
        
        /* 0x20 */ OP(MUL)  OP(DIV)  OP(MOD)  OP(LINENO)
        /* 0x24 */ OP(LOADTHIS)  OP(LOADUP)  OP(STOREUP)  OP(CLOSURE)
        /* 0x28 */ OP(YIELD)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)
        /* 0x2c */ OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)  OP(UNKNOWN)

        /* 0x30 */ OP(UMINUS)  OP(UNOT)  OP(UNEG)  OP(PREINC)
        /* 0x34 */ OP(PREDEC)  OP(POSTINC)  OP(POSTDEC)  OP(CALL)
        /* 0x38 */ OP(NEW)  OP(CALLPROP) OP(JMP)  OP(JT)
        /* 0x3c */ OP(JF) OP(END) OP(RET) OP(UNKNOWN)
    };
 
    static_assert (sizeof(dispatchTable) == (1 << 6) * sizeof(void*), "Dispatch table is wrong size");

    #define DISPATCH goto *dispatchTable[static_cast<uint8_t>(dispatchNextOp(inst, checkCounter))]
    
    if (!_program) {
        return CallReturnValue(CallReturnValue::Type::Finished);
    }

    Object::gc();

    updateCodePointer();
    
    uint16_t checkCounter = 0;
    uint32_t uintValue;
    int32_t intValue;
    Float floatValue;
    bool boolValue;
    Value leftValue, rightValue;
    bool leftBoolValue, rightBoolValue;
    int32_t leftIntValue, rightIntValue;
    Float leftFloatValue, rightFloatValue;
    Mad<Object> objectValue;
    Mad<MaterObject> materObjectValue;
    Value returnedValue;
    CallReturnValue callReturnValue;
    uint32_t localsToPop;
    Mad<Object> prevThis;

    Instruction inst;
    
    DISPATCH;
    
    L_LINENO:
        _lineno = inst.un();
        DISPATCH;
        
    L_UNKNOWN:
        assert(0);
        return CallReturnValue(CallReturnValue::Type::Finished);

    L_YIELD:
        callReturnValue = CallReturnValue(CallReturnValue::Type::Yield);
    L_CHECK_EVENTS:
        if (!_eventQueue.empty()) {
            callReturnValue = runNextEvent();
            if (callReturnValue.isError()) {
                printError(callReturnValue.error());
                _terminate = true; 
                _stack.clear();
                Object::gc(true);
                return CallReturnValue(CallReturnValue::Type::Terminated);
            }
            return callReturnValue;
        }
        return callReturnValue;

    L_RET:
    L_RETX:
    L_END:
        if (_terminate) {
            _stack.clear();
            Object::gc(true);
            return CallReturnValue(CallReturnValue::Type::Terminated);
        }

        if (inst.op() == Op::END || (inst.op() == Op::RET && _callRecords.empty())) {
            // If _callRecords is empty it means we're returning from the top-level program.
            // TODO: How do we return this to the caller? What does the caller do with it.
            // This is essentially the same as the exit code returned from main() in C
            if (inst.op() == Op::RET) {
                // Take care of the values on the return stack
                returnedValue = (inst.nparams()) ? _stack.top(1 - inst.nparams()) : Value();
                _stack.pop(inst.nparams());
            }
            if (_program == _function) {
                // We've hit the end of the program
                
                if (!_stack.validateFrame(0, _program->localSize())) {
                    printError(ROMSTR("internal error. On exit stack has %d elements, should have %d"), _stack.size(), _program->localSize());
                    _terminate = true;
                    _stack.clear();
                    Object::gc(true);
                    return CallReturnValue(CallReturnValue::Type::Terminated);
                }
                
                Object::gc(true);
                return CallReturnValue(CallReturnValue::Type::Finished);
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
        if (!leftValue.isCallable()) {
            printError(ROMSTR("Can't read property '%s' of a non-callable value"), regOrConst(inst.rc()).toStringValue(this).c_str());
        } else {
            leftValue = regOrConst(inst.rb()).property(this, regOrConst(inst.rc()).toIdValue(this));
            if (!leftValue) {
                printError(ROMSTR("Property '%s' does not exist"), regOrConst(inst.rc()).toStringValue(this).c_str());
            } else {
                setInFrame(inst.ra(), leftValue);
            }
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
            printError(ROMSTR("Can't read element '%s' of a non-existant object"), regOrConst(inst.rc()).toStringValue(this).c_str());
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
        materObjectValue = Mad<MaterObject>::create();
        materObjectValue->setArray(true);
        setInFrame(inst.ra(), Value(materObjectValue));
        DISPATCH;
    L_LOADLITO:
        objectValue = Mad<MaterObject>::create();
        setInFrame(inst.ra(), Value(objectValue));
        DISPATCH;
    L_APPENDPROP:
        if (!reg(inst.ra()).setProperty(this, regOrConst(inst.rb()).toIdValue(this), regOrConst(inst.rc()), Value::SetPropertyType::AlwaysAdd)) {
            printError(ROMSTR("Property '%s' already exists for APPENDPROP"), regOrConst(inst.rb()).toStringValue(this).c_str());
        }
        DISPATCH;
    L_APPENDELT:
        if (!reg(inst.ra()).setElement(this, Value(), regOrConst(inst.rb()), true)) {
            printError(ROMSTR("Can't append element '%s' to object"), regOrConst(inst.rb()).toStringValue(this).c_str());
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
    L_LOR:
    L_LAND:
        leftBoolValue = regOrConst(inst.rb()).toBoolValue(this);
        rightBoolValue = regOrConst(inst.rc()).toBoolValue(this);
        setInFrame(inst.ra(), (inst.op() == Op::LOR) ? Value(leftBoolValue || rightBoolValue) : Value(leftBoolValue && rightBoolValue));
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
        setInFrame(inst.ra(), Value(compareValues(regOrConst(inst.rb()), regOrConst(inst.rc())) == 0));
        DISPATCH;
    L_NE: 
        setInFrame(inst.ra(), Value(compareValues(regOrConst(inst.rb()), regOrConst(inst.rc())) != 0));
        DISPATCH;
    L_LT: 
        setInFrame(inst.ra(), Value(compareValues(regOrConst(inst.rb()), regOrConst(inst.rc())) < 0));
        DISPATCH;
    L_LE: 
        setInFrame(inst.ra(), Value(compareValues(regOrConst(inst.rb()), regOrConst(inst.rc())) <= 0));
        DISPATCH;
    L_GT: 
        setInFrame(inst.ra(), Value(compareValues(regOrConst(inst.rb()), regOrConst(inst.rc())) > 0));
        DISPATCH;
    L_GE: 
        setInFrame(inst.ra(), Value(compareValues(regOrConst(inst.rb()), regOrConst(inst.rc())) >= 0));
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
            Mad<String> string = Object::createString(leftValue.toStringValue(this) + rightValue.toStringValue(this));
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
        Mad<Closure> closure = Mad<Closure>::create();
        closure->init(this, regOrConst(inst.rb()), _this ? Value(_this) : Value());
        setInFrame(inst.ra(), Value(static_cast<Mad<Object>>(closure)));
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
                if (callReturnValue.isError()) {
                    printError("'%s'", _program->stringFromAtom(name).c_str());
                }
                break;
        }
        
        if (callReturnValue.isError()) {
            printError(callReturnValue.error());
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
        if (callReturnValue.isMsDelay() || callReturnValue.isWaitForEvent()) {
            goto L_CHECK_EVENTS;
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
