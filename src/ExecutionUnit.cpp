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
#include "GC.h"
#include "MStream.h"
#include "Parser.h"
#include "SystemInterface.h"
#include "SystemTime.h"

using namespace m8r;

static const Duration EvalDurationMax = 2_sec;

void ExecutionUnit::tooManyErrors() const
{
    printf(ROMSTR("\n\nToo many runtime errors, (%d) exiting...\n"), _nerrors);
}

void ExecutionUnit::printError(ROMString format, ...) const
{
    va_list args;
    va_start(args, format);
    printf(ROMSTR("***** "));

    Error::vprintError(this, Error::Code::RuntimeError, _lineno, format, args);
    ++_nerrors;
}

void ExecutionUnit::printError(CallReturnValue::Error error) const
{
    ROMString errorString = ROMSTR("*UNKNOWN*");
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
    if (!_program.valid()) {
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
    if (_this.valid()) {
        Value oldValue = _this->property(atom);
        if (oldValue) {
            if (!_this->setProperty(atom, value, Value::SetPropertyType::AddIfNeeded)) {
                printError(ROMSTR("'%s' property of this object cannot be set"), _program->stringFromAtom(atom).c_str());
            }
            return;
        }
    }

    // See if it's in Program
    Value oldValue = _program->property(atom);
    if (oldValue) {
        if (!_program->setProperty(atom, value, Value::SetPropertyType::AddIfNeeded)) {
            printError(ROMSTR("'%s' property of this object cannot be set"), _program->stringFromAtom(atom).c_str());
        }
        return;
    }

    printError(ROMSTR("'%s' property does not exist or cannot be set"), _program->stringFromAtom(atom).c_str());
    return;
}

Value ExecutionUnit::derefId(Atom atom)
{
    if (!atom) {
        printError(ROMSTR("Value in LOADREFK must be an Atom"));
        return Value();
    }
    
    // Look in this then program then global
    Value value;
    if (_this.valid()) {
        value = _this->property(atom);
        if (value) {
            return value;
        }
    }

    value = _program->property(atom);
    if (value) {
        return value;
    }
    
    value = _program->global()->property(atom);
    if (value) {
        return value;
    }
    
    printError(ROMSTR("'%s' property does not exist in global scope"), _program->stringFromAtom(atom).c_str());
    return Value();
}

void ExecutionUnit::startExecution(Mad<Program> program)
{
    if (!program.valid()) {
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

    _openUpValues.clear();
}

void ExecutionUnit::fireEvent(const Value& func, const Value& thisValue, const Value* args, int32_t nargs)
{
    eventLock();
    
    _eventQueue.push_back(func);
    _eventQueue.push_back(thisValue);
    _eventQueue.push_back(Value(nargs));
    for (int i = 0; i < nargs; i++) {
        _eventQueue.push_back(args[i]);
    }

    eventUnlock();
}

void ExecutionUnit::receivedData(const String& data, Telnet::Action action)
{
    // Get the consoleListener from Program and use that to fire an event
    Value listener = program()->property(Atom(SA::consoleListener));
    if (listener && !listener.isNull()) {
        Value args[2];
        args[0] = Value(String::create(data));

        // Action is an enum, but it is always a 4 character string encoded as a uint32_t.
        // It may have trailing spaces. Convert it to a StringLiteral
        char a[5];
        uint32_t actionInt = static_cast<uint32_t>(action);
        a[0] = actionInt >> 24;
        a[1] = actionInt >> 16;
        a[2] = actionInt >> 8;
        a[3] = actionInt;
        a[4] = '\0';
        for (auto& c : a) {
            if (c <= ' ') {
                c = '\0';
                break;
            }
        }
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
    
    eventLock();

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

    eventUnlock();
    
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

void ExecutionUnit::vprintf(ROMString format, va_list args) const
{
    String s = String::vformat(format, args);

    if (consolePrintFunction()) {
        consolePrintFunction()(s);
    } else {
        system()->printf(ROMSTR("%s"), s.c_str());
    }
}

void ExecutionUnit::print(const char* s) const
{
    printf(ROMSTR("%s"), s);
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

void ExecutionUnit::closeUpValues(uint32_t frame)
{
    auto it = std::remove_if(_openUpValues.begin(), _openUpValues.end(), 
        [this, frame](const Mad<UpValue>& upValue) {
            return upValue->closeIfNeeded(this, frame);
        });
    
    _openUpValues.erase(it, _openUpValues.end());
}

void ExecutionUnit::startFunction(Mad<Object> function, Mad<Object> thisObject, uint32_t nparams)
{
    assert(_program.valid());
    assert(function.valid());
    
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
    if (!_this.valid()) {
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
        return 0;
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
    Mad<Function> parent = Object::create<Function>();
    
    Mad<Function> function = parser.parse(stream, this, Parser::Debug::Full, parent);
    if (parser.nerrors()) {
        syntaxErrors.swap(parser.syntaxErrors());
        
        // TODO: Do something with syntaxErrors
        return CallReturnValue(CallReturnValue::Error::SyntaxErrors);
    }
    
    // Get all the contents into a new object
    Mad<Object> obj = Object::create<MaterObject>();
    
    // Get any constant functions
    for (auto it : *(function->constants())) {
        Mad<Function> func = it.asFunction();
        if (func.valid() && func->name()) {
            obj->setProperty(func->name(), Value(func), Value::SetPropertyType::AlwaysAdd);
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

    #define DISPATCH { op = dispatchNextOp(checkCounter); goto *dispatchTable[static_cast<uint8_t>(op)]; }
    
    if (!_program.valid()) {
        return CallReturnValue(CallReturnValue::Type::Finished);
    }
    
    GC::gc();

    updateCodePointer();
    
    uint16_t checkCounter = 0;
    uint32_t uintValue;
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
    Atom prop;
    uint8_t ra, rb;
    
    Op op = Op::UNKNOWN;
    
    DISPATCH;
    
    L_LINENO:
        _lineno = uNFromCode();
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
                GC::gc(true);
                return CallReturnValue(CallReturnValue::Type::Terminated);
            }
            return callReturnValue;
        }
        return callReturnValue;

    L_RET:
    L_END:
        if (_terminate) {
            _stack.clear();
            _callRecords.clear();
            GC::gc(true);
            return CallReturnValue(CallReturnValue::Type::Terminated);
        }

        if (op == Op::END || (op == Op::RET && _callRecords.empty())) {
            // If _callRecords is empty it means we're returning from the top-level program.
            // TODO: How do we return this to the caller? What does the caller do with it.
            // This is essentially the same as the exit code returned from main() in C
            if (op == Op::RET) {
                // Take care of the values on the return stack
                uint8_t nparams = byteFromCode();
                returnedValue = nparams ? _stack.top(1 - nparams) : Value();
                _stack.pop(nparams);
            }
            if (_program == _function) {
                // We've hit the end of the program
                
                if (!_stack.validateFrame(0, _program->localSize())) {
                    printError(ROMSTR("internal error. On exit stack has %d elements, should have %d"), _stack.size(), _program->localSize());
                    _terminate = true;
                    _stack.clear();
                    GC::gc(true);
                    return CallReturnValue(CallReturnValue::Type::Terminated);
                }
                
                GC::gc(true);
                return CallReturnValue(CallReturnValue::Type::Finished);
            }
            callReturnValue = CallReturnValue();
        }
        else {
            // Assume this is RET
            uint8_t nparams = byteFromCode();
            callReturnValue = CallReturnValue(CallReturnValue::Type::ReturnCount, nparams);
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
            assert(_this.valid());
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
        setInFrame(byteFromCode(), regOrConst(byteFromCode()));
        DISPATCH;
    L_LOADREFK:
        setInFrame(byteFromCode(), derefId(regOrConst(byteFromCode()).asIdValue()));
        DISPATCH;
    L_STOREFK:
        stoIdRef(regOrConst(byteFromCode()).asIdValue(), regOrConst(byteFromCode()));
        DISPATCH;
    L_LOADPROP:
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode()).property(this, (rightValue = regOrConst(byteFromCode())).toIdValue(this));
        // TODO: Distinguish between Values that can't have properties and those that can
        //
        // We don't distinguish between Values that can't have properties (like Id and NativeFunction) and
        // Those that just don't happen to have a property with the given name. Maybe have an error Value
        // returned that will tell us that?
        if (!leftValue) {
            // We need to handle 'iterator' here because if the value is undefined and the property
            // is 'iterator' we need to supply the default iterator
            if (prop == Atom(SA::iterator)) {
                leftValue = program()->global()->property(Atom(SA::Iterator));
            } else {
                printError(ROMSTR("Property '%s' does not exist"), rightValue.toStringValue(this).c_str());
                DISPATCH;
            }
        }
        setInFrame(ra, leftValue);
        DISPATCH;
    L_STOPROP:
        if (!reg(byteFromCode()).setProperty(this, (leftValue = regOrConst(byteFromCode())).toIdValue(this), regOrConst(byteFromCode()), Value::SetPropertyType::NeverAdd)) {
            printError(ROMSTR("Property '%s' does not exist"), leftValue.toStringValue(this).c_str());
        }
        DISPATCH;
    L_LOADELT:
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode()).element(this, (rightValue = regOrConst(byteFromCode())));
        if (!leftValue) {
            printError(ROMSTR("Can't read element '%s' of a non-existant object"), rightValue.toStringValue(this).c_str());
        } else {
            setInFrame(ra, leftValue);
        }
        DISPATCH;
    L_STOELT:
        if (!reg(byteFromCode()).setElement(this, (leftValue = regOrConst(byteFromCode())), regOrConst(byteFromCode()), false)) {
            printError(ROMSTR("Element '%s' does not exist"), leftValue.toStringValue(this).c_str());
        }
        DISPATCH;
    L_LOADUP:
        ra = byteFromCode();
        if (!_function->loadUpValue(this, byteFromCode(), rightValue)) {
            printError(ROMSTR("unable to load upValue"));
        } else {
            setInFrame(ra, rightValue);
        }
        DISPATCH;
    L_STOREUP:
        if (!_function->storeUpValue(this, byteFromCode(), regOrConst(byteFromCode()))) {
            printError(ROMSTR("unable to store upValue"));
        }
        DISPATCH;
    L_LOADLITA:
        materObjectValue = Object::create<MaterObject>();
        materObjectValue->setArray(true);
        setInFrame(byteFromCode(), Value(materObjectValue));
        DISPATCH;
    L_LOADLITO:
        objectValue = Object::create<MaterObject>();
        setInFrame(byteFromCode(), Value(objectValue));
        DISPATCH;
    L_APPENDPROP:
        if (!reg(byteFromCode()).setProperty(this, (leftValue = regOrConst(byteFromCode())).toIdValue(this), regOrConst(byteFromCode()), Value::SetPropertyType::AlwaysAdd)) {
            printError(ROMSTR("Property '%s' already exists for APPENDPROP"), leftValue.toStringValue(this).c_str());
        }
        DISPATCH;
    L_APPENDELT:
        if (!reg(byteFromCode()).setElement(this, Value(), (leftValue = regOrConst(byteFromCode())), true)) {
            printError(ROMSTR("Can't append element '%s' to object"), leftValue.toStringValue(this).c_str());
        }
        DISPATCH;
    L_LOADTRUE:
        setInFrame(byteFromCode(), Value(true));
        DISPATCH;
    L_LOADFALSE:
        setInFrame(byteFromCode(), Value(false));
        DISPATCH;
    L_LOADNULL:
        setInFrame(byteFromCode(), Value());
        DISPATCH;
    L_LOADTHIS:
        setInFrame(byteFromCode(), Value(_this));
        DISPATCH;
    L_PUSH:
        _stack.push(regOrConst(byteFromCode()));
        DISPATCH;
    L_POP:
        setInFrame(byteFromCode(), _stack.top());
        _stack.pop();
        DISPATCH;
    L_LOR:
    L_LAND:
        ra = byteFromCode();
        leftBoolValue = regOrConst(byteFromCode()).toBoolValue(this);
        rightBoolValue = regOrConst(byteFromCode()).toBoolValue(this);
        setInFrame(ra, (op == Op::LOR) ? Value(leftBoolValue || rightBoolValue) : Value(leftBoolValue && rightBoolValue));
        DISPATCH;
    L_BINIOP:
        ra = byteFromCode();
        leftIntValue = regOrConst(byteFromCode()).toIntValue(this);
        rightIntValue = regOrConst(byteFromCode()).toIntValue(this);
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
        setInFrame(ra, Value(leftIntValue));
        DISPATCH;
    L_EQ: 
        setInFrame(byteFromCode(), Value(compareValues(regOrConst(byteFromCode()), regOrConst(byteFromCode())) == 0));
        DISPATCH;
    L_NE: 
        setInFrame(byteFromCode(), Value(compareValues(regOrConst(byteFromCode()), regOrConst(byteFromCode())) != 0));
        DISPATCH;
    L_LT: 
        setInFrame(byteFromCode(), Value(compareValues(regOrConst(byteFromCode()), regOrConst(byteFromCode())) < 0));
        DISPATCH;
    L_LE: 
        setInFrame(byteFromCode(), Value(compareValues(regOrConst(byteFromCode()), regOrConst(byteFromCode())) <= 0));
        DISPATCH;
    L_GT: 
        setInFrame(byteFromCode(), Value(compareValues(regOrConst(byteFromCode()), regOrConst(byteFromCode())) > 0));
        DISPATCH;
    L_GE: 
        setInFrame(byteFromCode(), Value(compareValues(regOrConst(byteFromCode()), regOrConst(byteFromCode())) >= 0));
        DISPATCH;
    L_SUB:
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode());
        rightValue = regOrConst(byteFromCode());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(ra, Value(leftValue.asIntValue() - rightValue.asIntValue()));
        } else {
            setInFrame(ra, Value(leftValue.toFloatValue(this) - rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_MUL:
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode());
        rightValue = regOrConst(byteFromCode());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(ra, Value(leftValue.asIntValue() * rightValue.asIntValue()));
        } else {
            setInFrame(ra, Value(leftValue.toFloatValue(this) * rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_DIV:
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode());
        rightValue = regOrConst(byteFromCode());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(ra, Value(leftValue.asIntValue() / rightValue.asIntValue()));
        } else {
            setInFrame(ra, Value(leftValue.toFloatValue(this) / rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_MOD: 
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode());
        rightValue = regOrConst(byteFromCode());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(ra, Value(leftValue.asIntValue() % rightValue.asIntValue()));
        } else {
            setInFrame(ra, Value(leftValue.toFloatValue(this) % rightValue.toFloatValue(this)));
        }
        DISPATCH;
    L_ADD:
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode());
        rightValue = regOrConst(byteFromCode());
        if (valuesAreInt(leftValue, rightValue)) {
            setInFrame(ra, Value(leftValue.asIntValue() + rightValue.asIntValue()));
        } else if (leftValue.isNumber() && rightValue.isNumber()) {
            setInFrame(ra, Value(leftValue.toFloatValue(this) + rightValue.toFloatValue(this)));
        } else {
            Mad<String> string = String::create(leftValue.toStringValue(this) + rightValue.toStringValue(this));
            setInFrame(ra, Value(string));
        }
        DISPATCH;
    L_UMINUS:
        ra = byteFromCode();
        leftValue = regOrConst(byteFromCode());
        if (leftValue.isInteger()) {
            setInFrame(ra, Value(-leftValue.asIntValue()));
        } else {
            setInFrame(ra, Value(-leftValue.toFloatValue(this)));
        }
        DISPATCH;
    L_UNEG:
        setInFrame(byteFromCode(), Value((regOrConst(byteFromCode()).toIntValue(this) == 0) ? 1 : 0));
        DISPATCH;
    L_UNOT:
        setInFrame(byteFromCode(), Value(~(regOrConst(byteFromCode()).toIntValue(this))));
        DISPATCH;
    L_PREINC:
        ra = byteFromCode();
        rb = byteFromCode();
        setInFrame(rb, Value(regOrConst(rb).toIntValue(this) + 1));
        setInFrame(ra, regOrConst(rb));
        DISPATCH;
    L_PREDEC:
        ra = byteFromCode();
        rb = byteFromCode();
        setInFrame(rb, Value(regOrConst(rb).toIntValue(this) - 1));
        setInFrame(ra, regOrConst(rb));
        DISPATCH;
    L_POSTINC:
        ra = byteFromCode();
        rb = byteFromCode();
        setInFrame(ra, regOrConst(rb));
        setInFrame(rb, Value(regOrConst(rb).toIntValue(this) + 1));
        DISPATCH;
    L_POSTDEC:
        ra = byteFromCode();
        rb = byteFromCode();
        setInFrame(ra, regOrConst(rb));
        setInFrame(rb, Value(regOrConst(rb).toIntValue(this) - 1));
        DISPATCH;
    L_CLOSURE: {
        ra = byteFromCode();
        Mad<Closure> closure = Object::create<Closure>();
        closure->init(this, regOrConst(byteFromCode()), _this.valid() ? Value(_this) : Value());
        setInFrame(ra, Value(static_cast<Mad<Object>>(closure)));
        DISPATCH;
    }
    L_NEW:
    L_CALL:
    L_CALLPROP: {
        ra = byteFromCode();
        rb = (op != Op::NEW) ? byteFromCode() : 0;
        
        leftValue = regOrConst(ra);
        uintValue = byteFromCode();
        Atom name;

        switch(op) {
            default: break;
            case Op::CALL: {
                rightValue = regOrConst(rb);
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
                name = regOrConst(rb).asIdValue();
                callReturnValue = leftValue.callProperty(this, name, uintValue);
                if (callReturnValue.isError()) {
                    printError(ROMSTR("'%s'"), _program->stringFromAtom(name).c_str());
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
        if (op != Op::JMP) {
            boolValue = regOrConst(byteFromCode()).toBoolValue(this);
            if (op == Op::JT) {
                boolValue = !boolValue;
            }
            if (boolValue) {
                sNFromCode();
                DISPATCH;
            }
        }
        _pc += sNFromCode() - 1;
        DISPATCH;
}

String ExecutionUnit::debugString(uint16_t index)
{
    return _program->stringFromAtom(Atom(index));
}

