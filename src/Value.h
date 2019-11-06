/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Atom.h"
#include "Error.h"
#include "SystemTime.h"

namespace m8r {

class NativeObject;
class Object;
class Function;
class ExecutionUnit;
class Program;
class Value;

class CallReturnValue {
public:
    // Values:
    //
    //      -6M -1      - delay in -ms
    //      0-999       - return count
    //      1000        - function start
    //      1001        - program finished
    //      1002        - program terminated
    //      1003        - wait for event
    //      1004        - continue execution (yield)
    //      1005-1999   - unused
    //      2000...     - error codes
    static constexpr int32_t MaxReturnCount = 999;
    static constexpr int32_t FunctionStartValue = 1000;
    static constexpr int32_t FinishedValue = 1001;
    static constexpr int32_t TerminatedValue = 1002;
    static constexpr int32_t WaitForEventValue = 1003;
    static constexpr int32_t YieldValue = 1004;
    static constexpr int32_t MaxMsDelay = 6000000;

    static constexpr int32_t ErrorValue = 2000;
    
    enum class Type { ReturnCount = 0, MsDelay = 1, FunctionStart, Finished, Terminated, WaitForEvent, Yield };
    
    enum class Error {
        Ok,
        WrongNumberOfParams,
        ConstructorOnly,
        Unimplemented,
        OutOfRange,
        MissingThis,
        InternalError,
        PropertyDoesNotExist,
        BadFormatString,
        UnknownFormatSpecifier,
        CannotConvertStringToNumber,
        CannotCreateArgumentsArray,
        CannotCall,
        InvalidArgumentValue,
        SyntaxErrors,
        ImportTimeout,
        DelayNotAllowedInImport,
        EventNotAllowedInImport,
        Error,
    };
    
    CallReturnValue(Type type = Type::ReturnCount, uint32_t value = 0)
    {
        switch(type) {
            case Type::MsDelay:
                 assert(value >= 0 && value <= MaxMsDelay);
                 
                 // If a 0 delay was passed, handle this like a yield
                 _value = (value == 0) ? YieldValue : -value;
                 break;
            case Type::ReturnCount: assert(value <= MaxReturnCount); _value = value; break;
             case Type::FunctionStart: _value = FunctionStartValue; break;
            case Type::Finished: _value = FinishedValue; break;
            case Type::Terminated: _value = TerminatedValue; break;
            case Type::WaitForEvent: _value = WaitForEventValue; break;
            case Type::Yield: _value = YieldValue; break;
        }
    }
    
    CallReturnValue(Error error) { _value = ErrorValue + static_cast<int32_t>(error); }
    
    bool isFunctionStart() const { return _value == FunctionStartValue; }
    bool isError() const { return _value >= ErrorValue; }
    bool isFinished() const { return _value == FinishedValue; }
    bool isTerminated() const { return _value == TerminatedValue; }
    bool isWaitForEvent() const { return _value == WaitForEventValue; }
    bool isYield() const { return _value == YieldValue; }
    bool isReturnCount() const { return _value >= 0 && _value <= MaxReturnCount; }
    bool isMsDelay() const { return _value < 0 && _value >= -MaxMsDelay; }
    Duration msDelay() const { assert(isMsDelay()); return Duration(-_value, Duration::Units::ms); }
    uint32_t returnCount() const { assert(isReturnCount()); return _value; }
    Error error() const { return isError() ? static_cast<Error>(_value - ErrorValue) : Error::Error; }

private:
    int32_t _value = 0;
};

using NativeFunction = CallReturnValue(*)(ExecutionUnit*, Value thisValue, uint32_t nparams);

class Value {
public:
    typedef m8r::Map<Atom, Value> Map;
    
    enum class Type : uint32_t {
        None = 0,
        Float = 1,
        Object = 2,
        Function = 4,
        Integer = 6,
        String = 8,
        StringLiteral = 10,
        Id = 12,
        Null = 14,
        NativeObject = 16,
        NativeFunction = 18,
    };

    Value() { _value._raw = 0; _value._type = Type::None; }
    
    explicit Value(Float value) { _value._float = value.raw(); _value._float |= 0x01; }
    
    explicit Value(Object* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::Object; }
    explicit Value(Function* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::Function; }
    explicit Value(String* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::String; }
    explicit Value(NativeObject* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::NativeObject; }
    explicit Value(NativeFunction value) { assert(value); _value._raw = 0; _value._callable = value; _value._type = Type::NativeFunction; }

    explicit Value(int32_t value) { _value._raw = 0; _value._int = value; _value._type = Type::Integer; }
    explicit Value(Atom value) { _value._raw = 0; _value._int = value.raw(); _value._type = Type::Id; }
    explicit Value(StringLiteral value) { _value._raw = 0; _value._int = value.raw(); _value._type = Type::StringLiteral; }
    
    // Define these to make sure no implicit functions are being called
    Value(const Value& other) { _value._raw = other._value._raw; }
    Value(Value&& other) { _value._raw = other._value._raw; }
    Value& operator=(const Value& other) { _value._raw = other._value._raw; return *this; }
    Value& operator=(Value&& other) { _value._raw = other._value._raw; return *this; }
    
    static Value NullValue() { Value value; value._value._raw = 0; value._value._type = Type::Null; return value; }
    
    bool operator==(const Value& other) { return _value._raw == other._value._raw; }
    bool operator!=(const Value& other) { return _value._raw != other._value._raw; }
    
    explicit operator bool() const { return type() != Type::None; }

    ~Value() { }
    
    Type type() const { return ((_value._float & 0x01) == 0) ? _value._type : Type::Float; }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    Object* asObject() const { return (type() == Type::Object || type() == Type::Function) ? objectFromValue() : nullptr; }
    Function* asFunction() const { return (type() == Type::Function) ? functionFromValue() : nullptr; }
    String* asString() const { return (type() == Type::String) ? stringFromValue() : nullptr; }
    StringLiteral asStringLiteralValue() const { return (type() == Type::StringLiteral) ? stringLiteralFromValue() : StringLiteral(); }
    int32_t asIntValue() const { return (type() == Type::Integer) ? int32FromValue() : 0; }
    Float asFloatValue() const { return (type() == Type::Float) ? floatFromValue() : Float(); }
    Atom asIdValue() const { return (type() == Type::Id) ? atomFromValue() : Atom(); }
    NativeObject* asNativeObject() const { return (type() == Type::NativeObject) ? nativeObjectFromValue() : nullptr; }
    NativeFunction asNativeFunction() { return (type() == Type::NativeFunction) ? nativeFunctionFromValue() : nullptr; }

    m8r::String toStringValue(ExecutionUnit*) const;
    bool toBoolValue(ExecutionUnit* eu) const
    {
        switch (type()) {
            default:
            case Type::Null:
            case Type::None:              return false;
            case Type::Object:
            case Type::NativeObject:
            case Type::Function:          return _value._ptr;
            case Type::NativeFunction:    return _value._callable;
            case Type::Integer:           return int32FromValue() != 0;
            case Type::Float:
            case Type::String:
            case Type::StringLiteral:
            case Type::Id:                return toIntValue(eu) != 0;
        }
    }
    
    Float toFloatValue(ExecutionUnit* eu) const
    {
        return (type() == Type::Float) ? floatFromValue() : ((type() == Type::Integer) ? Float(int32FromValue()) : _toFloatValue(eu));
    }

    int32_t toIntValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Integer) {
            return int32FromValue();
        }
        return static_cast<int32_t>(toFloatValue(eu));
    }
    
    Atom toIdValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Id) {
            return atomFromValue();
        }
        return _toIdValue(eu);
    }
        
    bool isNull() const { return type() == Type::Null; }
    bool isString() const { return type() == Type::String || type() == Type::StringLiteral; }
    bool isStringLiteral() const { return type() == Type::StringLiteral; }
    bool isInteger() const { return type() == Type::Integer; }
    bool isFloat() const { return type() == Type::Float; }
    bool isNumber() const { return isInteger() || isFloat(); }
    bool isNone() const { return type() == Type::None; }
    bool isObject() const { return type() == Type::Object; }
    bool isFunction() const { return type() == Type::Function; }
    bool isNativeObject() const { return type() == Type::NativeObject; }
    bool isNativeFunction() const { return type() == Type::NativeFunction; }
    bool isPointer() const { return isObject() || isFunction() || isNativeObject(); }
    
    bool isType(ExecutionUnit*, Atom);
    bool isType(ExecutionUnit*, SA);

    static String format(ExecutionUnit*, Value format, uint32_t nparams);
    
    void gcMark();
    
    enum class SetPropertyType { AlwaysAdd, NeverAdd, AddIfNeeded };

    const Value property(ExecutionUnit*, const Atom&) const;
    bool setProperty(ExecutionUnit*, const Atom& prop, const Value& value, Value::SetPropertyType);
    const Value element(ExecutionUnit* eu, const Value& elt) const;
    bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append);

    CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor);
    CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams);
        
    bool needsGC() const { return type() == Type::Object || type() == Type::String; }
    
private:
    Float _toFloatValue(ExecutionUnit*) const;
    Value _toValue(ExecutionUnit*) const;
    Atom _toIdValue(ExecutionUnit*) const;

    inline Float floatFromValue() const { return Float(static_cast<Float::value_type>(_value._float & ~1)); }
    inline int32_t int32FromValue() const { return _value._int; }
    inline uint32_t uint32FromValue() const { return _value._int; }
    inline Atom atomFromValue() const { return Atom(static_cast<Atom::value_type>(_value._int)); }
    inline String* stringFromValue() const { return reinterpret_cast<String*>(_value._ptr); }
    inline StringLiteral stringLiteralFromValue() const { return StringLiteral(static_cast<StringLiteral::value_type>(_value._int)); }
    inline NativeObject* nativeObjectFromValue() const { return reinterpret_cast<NativeObject*>(_value._ptr); }
    inline NativeFunction nativeFunctionFromValue() { return _value._callable; }
    inline Object* objectFromValue() const { return reinterpret_cast<Object*>(_value._ptr); }
    inline Function* functionFromValue() const { return reinterpret_cast<Function*>(_value._ptr); }

    union {
#ifdef __APPLE__
        __uint128_t _raw;
#else
        uint64_t _raw;
#endif
        uint64_t _float;
        struct {
            Type _type;
            union {
                union {
                    void* _ptr;
                    NativeFunction _callable;
                };
                int32_t _int;
            };
        };
    } _value;
    
    // In order to fit everything, we have some requirements
#ifdef __APPLE__
    static_assert(sizeof(_value) == 16, "Value on Mac must be 16 bytes");
#else
    static_assert(sizeof(_value) == 8, "Value on Esp must be 8 bytes");
#endif
};

}
