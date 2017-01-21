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

class Object;
class Value;
class ExecutionUnit;
class Program;
class Stream;

class CallReturnValue {
public:
    static constexpr uint32_t MaxReturnCount = 999;
    static constexpr uint32_t FunctionStartValue = 1000;
    static constexpr uint32_t ErrorValue = 1001;
    static constexpr uint32_t FinishedValue = 1002;
    static constexpr uint32_t WaitForEventValue = 1003;
    static constexpr uint32_t MaxMsDelay = 6000000;
    
    enum class Type { ReturnCount = 0, MsDelay = 1, FunctionStart = FunctionStartValue, Error = ErrorValue, Finished = FinishedValue, WaitForEvent = WaitForEventValue };
    
    CallReturnValue(Type type = Type::ReturnCount, uint32_t value = 0)
    {
        switch(type) {
            case Type::ReturnCount: assert(value <= MaxReturnCount); _value = value; break;
            case Type::MsDelay: assert(value <= MaxMsDelay); _value = -value; break;
            case Type::FunctionStart: _value = FunctionStartValue; break;
            case Type::Error: _value = ErrorValue; break;
            case Type::Finished: _value = FinishedValue; break;
            case Type::WaitForEvent: _value = WaitForEventValue; break;
        }
    }
    
    bool isFunctionStart() const { return _value == FunctionStartValue; }
    bool isError() const { return _value == ErrorValue; }
    bool isFinished() const { return _value == FinishedValue; }
    bool isWaitForEvent() const { return _value == WaitForEventValue; }
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
    
    enum class Type : uint8_t {
        None = 0,
        Object, Float, Integer, String, StringLiteral, Id,
        PreviousFrame, PreviousPC, PreviousObject, PreviousParamCount,
    };

    Value() { }
    Value(const Value& other) { _value._raw = other._value._raw; }
    Value(Value&& other) { _value._raw = other._value._raw; }
    
    Value(ObjectId objectId, Type type = Type::Object) : _value(objectId, type) { }
    Value(Float value) : _value(value) { }
    Value(int32_t value) : _value(value) { }
    Value(uint32_t value, Type type) : _value(value, type) { }
    Value(Atom value) : _value(value) { }
    Value(StringId stringId) : _value(stringId) { }
    Value(StringLiteral stringId) : _value(stringId) { }
    
    Value& operator=(const Value& other) { _value._raw = other._value._raw; return *this; }
    operator bool() const { return type() != Type::None; }
    operator uint64_t() const { return _value._raw; }
    bool operator==(const Value& other) { return _value._raw == other._value._raw; }
    bool operator!=(const Value& other) { return _value._raw != other._value._raw; }

    ~Value() { }
    
    bool serialize(Stream*, Error&) const
    {
        // FIXME: Implement
        return false;
    }

    bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&)
    {
        // FIXME: Implement
        return false;
    }

    Type type() const { return _value.type(); }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    ObjectId asObjectIdValue() const { return (type() == Type::Object || type() == Type::PreviousObject) ? objectIdFromValue() : ObjectId(); }
    StringId asStringIdValue() const { return (type() == Type::String) ? stringIdFromValue() : StringId(); }
    StringLiteral asStringLiteralValue() const { return (type() == Type::StringLiteral) ? stringLiteralFromValue() : StringLiteral(); }
    int32_t asIntValue() const { return (type() == Type::Integer) ? intFromValue() : 0; }
    uint32_t asPreviousPCValue() const { return (type() == Type::PreviousPC) ? intFromValue() : 0; }
    uint32_t asPreviousFrameValue() const { return (type() == Type::PreviousFrame) ? intFromValue() : 0; }
    uint32_t asPreviousParamCountValue() const { return (type() == Type::PreviousParamCount) ? intFromValue() : 0; }
    Float asFloatValue() const { return (type() == Type::Float) ? floatFromValue() : Float(); }
    Atom asIdValue() const { return (type() == Type::Id) ? atomFromValue() : Atom(); }
    
    m8r::String toStringValue(ExecutionUnit*) const;
    bool toBoolValue(ExecutionUnit* eu) const { return toIntValue(eu) != 0; }
    Float toFloatValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Float) {
            return floatFromValue();
        }
        return _toFloatValue(eu);
    }

    int32_t toIntValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Integer) {
            return intFromValue();
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
    bool isObjectId() const { return type() == Type::Object || type() == Type::PreviousObject; }
    
    static m8r::String toString(Float value);
    static m8r::String toString(int32_t value);
    static Float floatFromString(const char*);
    
    void gcMark(ExecutionUnit* eu) 
    {
        if (asObjectIdValue() || asStringIdValue()) {
            _gcMark(eu);
        }
    }
    
    CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    
    bool needsGC() const { return type() == Type::Object || type() == Type::PreviousObject || type() == Type::String; }
    
private:
    static constexpr uint8_t TypeBitCount = 4;
    static constexpr uint8_t TypeMask = (1 << TypeBitCount) - 1;
    
    Float _toFloatValue(ExecutionUnit*) const;
    Atom _toIdValue(ExecutionUnit*) const;
    void _gcMark(ExecutionUnit*);

    inline Float floatFromValue() const { return Float(static_cast<Float::value_type>(_value._raw & ~TypeMask)); }
    inline int32_t intFromValue() const { return _value._i; }
    inline Atom atomFromValue() const { return Atom(static_cast<Atom::value_type>(_value._i)); }
    inline ObjectId objectIdFromValue() const { return ObjectId(static_cast<ObjectId::value_type>(_value._i)); }
    inline StringId stringIdFromValue() const { return StringId(static_cast<StringId::value_type>(_value._i)); }
    inline StringLiteral stringLiteralFromValue() const { return StringLiteral(static_cast<StringLiteral::value_type>(_value._i)); }
    inline uint16_t indexFromValue() const { return _value._d; }
    
    struct RawValue {
        RawValue() { _raw = 0; }
        RawValue(uint64_t v) { _raw = v; }
        RawValue(Float f) { _raw = f.raw(); setType(Type::Float); }
        RawValue(int32_t i) { _raw = 0; _i = i; setType(Type::Integer); }
        RawValue(uint32_t i, Type type)
        {
            assert(type == Type::PreviousPC || type == Type::PreviousFrame || type == Type::PreviousParamCount);
            _raw = 0;
            _i = i;
            setType(type);
        }
        RawValue(Atom atom) { _raw = 0; _i = atom.raw(); setType(Type::Id); }
        RawValue(StringId id) { _raw = 0; _i = id.raw(); setType(Type::String); }
        RawValue(StringLiteral id) { _raw = 0; _i = id.raw(); setType(Type::StringLiteral); }
        RawValue(ObjectId id, Type type = Type::Object) { _raw = 0; _i = id.raw(); setType(type); }
        RawValue(ObjectId id, uint16_t index, Type type) { _raw = 0; _i = id.raw(); _d = index; setType(type); }
        
        Type type() const { return static_cast<Type>(_type); }
#ifdef __APPLE__
        void setType(Type type) { _type = type; }
#else
        void setType(Type type) { _type = static_cast<uint32_t>(type); }
#endif
        union {
            uint64_t _raw;
            struct {
#ifdef __APPLE__
                Type _type : TypeBitCount;
#else
                uint32_t _type : TypeBitCount;
#endif
                uint16_t _ : 12;
                uint16_t _d : 16;
                uint32_t _i : 32;
            };
        };
    };
    
    RawValue _value;
    
    static_assert(sizeof(RawValue) == 8, "RawValue must be 64 bits");
    
    // In order to fit everything, we have some requirements
    static_assert(sizeof(_value) >= sizeof(Float), "Value must be large enough to hold a Float");
    static_assert(sizeof(_value) * 8 >= sizeof(ObjectId) * 8 + sizeof(uint16_t) * 8 + TypeBitCount, "Value must be large enough to hold an ObjectId, a 16 bit index and a 4 bit type");
    static_assert(sizeof(_value) * 8 >= sizeof(StringLiteral) * 8 + TypeBitCount, "Value must be large enough to hold a StringLiteral and a 4 bit type");
};

}
