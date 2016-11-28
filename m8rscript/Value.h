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
        
typedef union {
    void* v;
    Float::value_type f;
    int32_t i;
    uint32_t u;
    const char* s;
    Atom::value_type a;
    ObjectId::value_type o;
} U;

static_assert(sizeof(U) <= sizeof(void*), "Value union must not be larger than void*");

inline static const char* duplicateString(const char* s, int32_t len = -1)
{
    size_t length = ((len < 0) ? strlen(s) : static_cast<size_t>(len)) + 1;
    char* newString = static_cast<char*>(malloc(length));
    assert(newString);
    if (newString) {
        memcpy(newString, s, length);
    }
    return newString;
}

class Value {
public:
    typedef m8r::Map<Atom, Value> Map;
    
    enum class Type : uint8_t {
        None = 0,
        Object, Float, Integer, String, Id,
        ElementRef, PropertyRef,
        PreviousFrame, PreviousPC, PreviousObject,
    };

    Value() : _value(nullptr), _type(Type::None), _id(0) { }
    Value(const Value& other) : _type(other._type), _value(other._value), _id(other._id) { }
    Value(Value&& other) : _type(other._type), _value(other._value), _id(other._id) { }
    
    Value(ObjectId objectId, Type type = Type::Object) : _value(valueFromObjectId(objectId)), _type(type), _id(0) { }
    Value(Float value) : _value(valueFromFloat(value)) , _type(Type::Float), _id(0) { }
    Value(int32_t value) : _value(valueFromInt(value)) , _type(Type::Integer), _id(0) { }
    Value(uint32_t value, Type type = Type::Integer) : _value(valueFromUInt(value)) , _type(type), _id(0) { }
    Value(Atom value) : _value(nullptr), _type(Type::Id), _id(value.raw()) { }
    Value(ObjectId objectId, uint16_t index, bool property) : _value(valueFromObjectId(objectId)), _type(property ? Type::PropertyRef : Type::ElementRef), _id(index) { }
    Value(const char* value, int32_t length = -1)
        : _value(valueFromStr(duplicateString(value, length)))
        , _type(Type::String), _id(0)
    { }
    
    // Steals the value pointer
    Value& operator=(Value&& other)
    {
        _value = other._value;
        _type = other._type;
        _id = other._id;
        return *this;
    }
    
    Value& operator=(const Value& other)
    {
        if (asStringValue()) {
            // FIXME: We need to manage these string copies
            //free(static_cast<void*>(const_cast<char*>(asStringValue())));
        }
        if (other.asStringValue()) {
            _value = valueFromStr(duplicateString(other.asStringValue()));
        } else {
            _value = other._value;
        }
        _type = other._type;
        _id = other._id;
        return *this;
    }
    
    operator bool() const { return _type != Type::None; }


    ~Value() {
        // FIXME: We need to manage these string copies
        if (asStringValue()) {
            //free(static_cast<void*>(const_cast<char*>(asStringValue())));
        }
    }
    
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

    Type type() const { return _type; }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    ObjectId asObjectIdValue() const { return (canBeBaked() || _type == Type::Object || _type == Type::PreviousObject) ? objectIdFromValue() : ObjectId(); }
    int32_t asIntValue() const { return (_type == Type::Integer) ? intFromValue() : 0; }
    uint32_t asUIntValue() const { return (_type == Type::Integer || _type == Type::PreviousPC || _type == Type::PreviousFrame) ? uintFromValue() : 0; }
    Float asFloatValue() const { return (_type == Type::Float) ? floatFromValue() : Float(); }
    Atom asIdValue() const { return (_type == Type::Id) ? Atom(_id) : Atom(); }

    const char* asStringValue() const { return (_type == Type::String) ? strFromValue() : nullptr; }
    
    m8r::String toStringValue(ExecutionUnit*) const;
    bool toBoolValue(ExecutionUnit*) const;
    Float toFloatValue(ExecutionUnit*) const;

    int32_t toIntValue(ExecutionUnit* eu) const { return static_cast<int32_t>(toUIntValue(eu)); }
    uint32_t toUIntValue(ExecutionUnit* eu) const
    {
        if (_type == Type::Integer) {
            return asUIntValue();
        }
        return canBeBaked() ? bake(eu).toUIntValue(eu) : static_cast<uint32_t>(toFloatValue(eu));
    }
    
    bool setValue(ExecutionUnit*, const Value&);
    Value bake(ExecutionUnit*) const;
    bool canBeBaked() const { return _type == Type::PropertyRef || _type == Type::ElementRef; }
    
    bool deref(ExecutionUnit*, const Value& derefValue);
    
    CallReturnValue call(ExecutionUnit*, uint32_t nparams);
    
    // FIXME: These functions must not be passed unbaked values
    bool isInteger() const
    {
        assert(!canBeBaked());
        return _type == Type::Integer;
    }
    bool isFloat() const
    {
        assert(!canBeBaked());
        return _type == Type::Float;
    }
    bool isNumber() const { return isInteger() || isFloat(); }
    
    bool isLValue() const { return canBeBaked(); }
    bool isNone() const { return _type == Type::None; }
    bool isAtom() const { return _type == Type::Id; }
    bool isObjectId() const { return _type == Type::Object || _type == Type::ElementRef || _type == Type::PropertyRef || _type == Type::PreviousObject; }
    
    static m8r::String toString(Float value);
    static m8r::String toString(int32_t value);
    static m8r::String toString(uint32_t value);
    static Float floatFromString(const char*);
    
private:
    inline void* valueFromFloat(Float f) const { U u; u.f = f.raw(); return u.v; }
    inline void* valueFromInt(int32_t i) const { U u; u.i = i; return u.v; }
    inline void* valueFromUInt(uint32_t i) const { U u; u.u = i; return u.v; }
    inline void* valueFromStr(const char* s) const { U u; u.s = s; return u.v; }
    inline void* valueFromObjectId(ObjectId id) const { U u; u.o = id.raw(); return u.v; }

    inline Float floatFromValue() const { U u; u.v = _value; return Float(u.f); }
    inline int32_t intFromValue() const { U u; u.v = _value; return u.i; }
    inline uint32_t uintFromValue() const { U u; u.v = _value; return u.u; }
    inline const char* strFromValue() const { U u; u.v = _value; return u.s; }
    inline ObjectId objectIdFromValue() const { U u; u.v = _value; return ObjectId(u.o); }
    
    // Assumes we already know this is an ObjectId
    Object* asObject(ExecutionUnit*) const;
    
    void* _value;
    Type _type;
    uint16_t _id;
};

}
