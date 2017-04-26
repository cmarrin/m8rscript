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

#pragma once

#include "Atom.h"
#include "Containers.h"
#include "Error.h"
#include "Float.h"

namespace m8r {

class NativeObject;
class Object;
class Function;
class ExecutionUnit;
class Program;

class CallReturnValue {
public:
    static constexpr uint32_t MaxReturnCount = 999;
    static constexpr uint32_t FunctionStartValue = 1000;
    static constexpr uint32_t ErrorValue = 1001;
    static constexpr uint32_t FinishedValue = 1002;
    static constexpr uint32_t TerminatedValue = 1003;
    static constexpr uint32_t WaitForEventValue = 1004;
    static constexpr uint32_t ContinueValue = 1004;
    static constexpr uint32_t MaxMsDelay = 6000000;
    
    enum class Type { ReturnCount = 0, MsDelay = 1, FunctionStart, Error, Finished, Terminated, WaitForEvent, Continue };
    
    CallReturnValue(Type type = Type::ReturnCount, uint32_t value = 0)
    {
        switch(type) {
            case Type::ReturnCount: assert(value <= MaxReturnCount); _value = value; break;
            case Type::MsDelay: assert(value > 0 && value <= MaxMsDelay); _value = -value; break;
            case Type::FunctionStart: _value = FunctionStartValue; break;
            case Type::Error: _value = ErrorValue; break;
            case Type::Finished: _value = FinishedValue; break;
            case Type::Terminated: _value = TerminatedValue; break;
            case Type::WaitForEvent: _value = WaitForEventValue; break;
            case Type::Continue: _value = ContinueValue; break;
        }
    }
    
    bool isFunctionStart() const { return _value == FunctionStartValue; }
    bool isError() const { return _value == ErrorValue; }
    bool isFinished() const { return _value == FinishedValue; }
    bool isTerminated() const { return _value == TerminatedValue; }
    bool isWaitForEvent() const { return _value == WaitForEventValue; }
    bool isContinue() const { return _value == ContinueValue; }
    bool isReturnCount() const { return _value >= 0 && _value <= MaxReturnCount; }
    bool isMsDelay() const { return _value < 0 && _value >= -MaxMsDelay; }
    uint32_t msDelay() const { assert(isMsDelay()); return -_value; }
    uint32_t returnCount() const { assert(isReturnCount()); return _value; }

private:
    int32_t _value = 0;
};
        
class Value {
public:
    typedef m8r::Map<Atom, Value> Map;
    
    enum class Type : uint32_t {
        None = 0,
        Float = 1,
        Object = 2, Function = 4, Integer = 6, String = 8, StringLiteral = 10, Id = 12, Null = 14, NativeObject = 16,
    };

    Value() { _value._float = 0; }
    
    explicit Value(Float value) { _value._float = value.raw(); _value._float |= 0x01; }
    
    explicit Value(Object* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::Object; }
    explicit Value(Function* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::Function; }
    explicit Value(String* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::String; }
    explicit Value(NativeObject* value) { assert(value); _value._raw = 0; _value._ptr = value; _value._type = Type::NativeObject; }
    
    explicit Value(int32_t value) { _value._raw = 0; _value._int = value; _value._type = Type::Integer; }
    explicit Value(Atom value) { _value._raw = 0; _value._int = value.raw(); _value._type = Type::Id; }
    explicit Value(StringLiteral value) { _value._raw = 0; _value._int = value.raw(); _value._type = Type::StringLiteral; }
    
    static Value NullValue() { Value value; value._value._raw = 0; value._value._type = Type::Null; return value; }
    
    bool operator==(const Value& other) { return _value._raw == other._value._raw; }
    bool operator!=(const Value& other) { return _value._raw != other._value._raw; }
    
    operator bool() const { return type() != Type::None; }

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
    
    m8r::String toStringValue(ExecutionUnit*) const;
    bool toBoolValue(ExecutionUnit* eu) const { return (type() == Type::Integer) ? (int32FromValue() != 0) : (toIntValue(eu) != 0); }
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
        
    bool isInteger() const { return type() == Type::Integer; }
    bool isFloat() const { return type() == Type::Float; }
    bool isNumber() const { return isInteger() || isFloat(); }
    bool isNone() const { return type() == Type::None; }
    bool isObject() const { return type() == Type::Object; }
    bool isFunction() const { return type() == Type::Function; }
    
    static m8r::String toString(Float value);
    static m8r::String toString(int32_t value);
    static bool toFloat(Float&, const char*, bool allowWhitespace = true);
    static bool toInt(int32_t&, const char*, bool allowWhitespace = true);
    static bool toUInt(uint32_t&, const char*, bool allowWhitespace = true);
    
    void gcMark(ExecutionUnit* eu);
    
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
                    Object* _obj;
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
