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
    enum class Type { ReturnCount, MsDelay, FunctionStart, Error };
    CallReturnValue(Type type = Type::ReturnCount, uint32_t value = 0)
    {
        switch(type) {
            case Type::ReturnCount: assert(value < 1000); _value = value; break;
            case Type::MsDelay: assert(value <= 6000000); _value = -value; break;
            case Type::FunctionStart: _value = 1000; break;
            case Type::Error: _value = 1001; break;
        }
    }
    
    bool isFunctionStart() const { return _value == 1000; }
    bool isError() const { return _value == 1001; }
    bool isReturnCount() const { return _value >= 0 && _value < 1000; }
    bool isMsDelay() const { return _value < 0 && _value >= -6000000; }
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
        ElementRef, PropertyRef,
        PreviousFrame, PreviousPC, PreviousObject,
    };

    Value() : _value(0) { }
    Value(const Value& other) : _value(other._value) { }
    Value(Value&& other) : _value(other._value) { }
    
    Value(ObjectId objectId, Type type = Type::Object) : _value(rawValue(objectId, type)) { }
    Value(Float value) : _value(rawValue(value, Type::Float)) { }
    Value(int32_t value) : _value(rawValue(value, Type::Integer)) { }
    Value(uint32_t value, Type type = Type::Integer) : _value(rawValue(value, type)) { }
    Value(Atom value) : _value(rawValue(value, Type::Id)) { }
    Value(ObjectId objectId, uint16_t index, bool property) : _value(rawValue(objectId, index, property ? Type::PropertyRef : Type::ElementRef)) { }
    Value(StringId stringId) : _value(rawValue(stringId, Type::String)) { }
    Value(StringLiteral stringId) : _value(rawValue(stringId, Type::StringLiteral)) { }
    
    Value& operator=(const Value& other) { _value = other._value; return *this; }
    operator bool() const { return type() != Type::None; }

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

    Type type() const { return static_cast<Type>(_value & TypeMask); }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    ObjectId asObjectIdValue() const { return (canBeBaked() || type() == Type::Object || type() == Type::PreviousObject) ? objectIdFromValue() : ObjectId(); }
    StringId asStringIdValue() const { return (type() == Type::String) ? stringIdFromValue() : StringId(); }
    StringLiteral asStringLiteralValue() const { return (type() == Type::StringLiteral) ? stringLiteralFromValue() : StringLiteral(); }
    int32_t asIntValue() const { return (type() == Type::Integer) ? intFromValue() : 0; }
    uint32_t asUIntValue() const { return (type() == Type::Integer || type() == Type::PreviousPC || type() == Type::PreviousFrame) ? uintFromValue() : 0; }
    Float asFloatValue() const { return (type() == Type::Float) ? floatFromValue() : Float(); }
    Atom asIdValue() const { return (type() == Type::Id) ? atomFromValue() : Atom(); }
    
    m8r::String toStringValue(ExecutionUnit*) const;
    bool toBoolValue(ExecutionUnit*) const;
    Float toFloatValue(ExecutionUnit*) const;

    uint32_t toUIntValue(ExecutionUnit* eu) const { return static_cast<uint32_t>(toUIntValue(eu)); }
    int32_t toIntValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Integer) {
            return asIntValue();
        }
        return static_cast<int32_t>(toFloatValue(eu));
    }
    
    bool setValue(ExecutionUnit*, const Value&);
    bool canBeBaked() const { return type() == Type::PropertyRef || type() == Type::ElementRef; }
    Value bake(ExecutionUnit* eu) const { return canBeBaked() ? _bake(eu) : *this; }
    
    bool deref(ExecutionUnit*, const Value& derefValue);
    
    CallReturnValue call(ExecutionUnit*, uint32_t nparams);
    
    // FIXME: These functions must not be passed unbaked values
    bool isInteger() const
    {
        assert(!canBeBaked());
        return type() == Type::Integer;
    }
    bool isFloat() const
    {
        assert(!canBeBaked());
        return type() == Type::Float;
    }
    bool isNumber() const { return isInteger() || isFloat(); }
    
    bool isLValue() const { return canBeBaked(); }
    bool isNone() const { return type() == Type::None; }
    bool isAtom() const { return type() == Type::Id; }
    bool isObjectId() const { return type() == Type::Object || type() == Type::ElementRef || type() == Type::PropertyRef || type() == Type::PreviousObject; }
    
    static m8r::String toString(Float value);
    static m8r::String toString(int32_t value);
    static m8r::String toString(uint32_t value);
    static Float floatFromString(const char*);
    
private:
    static constexpr uint8_t TypeBitCount = 4;
    static constexpr uint8_t TypeMask = (1 << TypeBitCount) - 1;
    
    Value _bake(ExecutionUnit*) const;

    bool derefObject(ExecutionUnit* eu, const Value& derefValue);

    inline uint64_t rawValue(Float f, Type t) const { return (static_cast<uint64_t>(f.raw()) & ~TypeMask) | static_cast<uint64_t>(t); }
    inline uint64_t rawValue(int32_t i, Type t) const { return (static_cast<uint64_t>(i) << TypeBitCount) | static_cast<uint64_t>(t); }
    inline uint64_t rawValue(uint32_t i, Type t) const { return (static_cast<uint64_t>(i) << TypeBitCount) | static_cast<uint64_t>(t); }
    inline uint64_t rawValue(Atom atom, Type t) const { return (static_cast<uint64_t>(atom.raw()) << TypeBitCount) | static_cast<uint64_t>(t); }
    inline uint64_t rawValue(ObjectId id, Type t) const { return (static_cast<uint64_t>(id.raw()) << TypeBitCount) | static_cast<uint64_t>(t); }
    inline uint64_t rawValue(ObjectId id, uint16_t index, Type t) const
    {
        return (static_cast<uint64_t>(index) << ((sizeof(ObjectId::value_type) * 8) + TypeBitCount)) | (static_cast<uint64_t>(id.raw()) << TypeBitCount) | static_cast<uint64_t>(t);
    }
    inline uint64_t rawValue(StringId id, Type t) const { return (static_cast<uint64_t>(id.raw()) << TypeBitCount) | static_cast<uint64_t>(t); }
    inline uint64_t rawValue(StringLiteral id, Type t) const { return (static_cast<uint64_t>(id.raw()) << TypeBitCount) | static_cast<uint64_t>(t); }

    inline Float floatFromValue() const { return Float(static_cast<Float::value_type>(_value & ~TypeMask)); }
    inline int32_t intFromValue() const { return static_cast<int32_t>(_value >> TypeBitCount); }
    inline uint32_t uintFromValue() const { return static_cast<uint32_t>(_value >> TypeBitCount); }
    inline uint16_t eltIndexFromValue() const { return static_cast<uint16_t>(_value >> TypeBitCount); }
    inline Atom atomFromValue() const { return Atom(static_cast<Atom::value_type>(_value >> TypeBitCount)); }
    inline ObjectId objectIdFromValue() const { return ObjectId(static_cast<ObjectId::value_type>(_value >> TypeBitCount)); }
    inline StringId stringIdFromValue() const { return StringId(static_cast<StringId::value_type>(_value >> TypeBitCount)); }
    inline StringLiteral stringLiteralFromValue() const { return StringLiteral(static_cast<StringLiteral::value_type>(_value >> TypeBitCount)); }
    inline uint16_t indexFromValue() const { return static_cast<uint16_t>(_value >> (16 + TypeBitCount)); }
    
    uint64_t _value;
    
    // In order to fit everything, we have some requirements
    static_assert(sizeof(_value) >= sizeof(Float), "Value must be large enough to hold a Float");
    static_assert(sizeof(_value) * 8 >= sizeof(ObjectId) * 8 + sizeof(uint16_t) * 8 + TypeBitCount, "Value must be large enough to hold an ObjectId, a 16 bit index and a 4 bit type");
    static_assert(sizeof(_value) * 8 >= sizeof(StringLiteral) * 8 + TypeBitCount, "Value must be large enough to hold a StringLiteral and a 4 bit type");
};

}
