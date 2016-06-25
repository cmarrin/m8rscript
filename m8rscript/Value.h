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
#include "Float.h"

namespace m8r {

class Object;
class Value;

typedef union {
    void* v;
    RawFloat f;
    int32_t i;
    uint32_t u;
    Object* o;
    const char* s;
    Atom::Raw a;
    Value* val;
} U;

class Value {
public:
    typedef m8r::Map<Atom, Value> Map;
    
    enum class Type : uint8_t { None = 0, Object, Float, Integer, String, Id, ElementRef, PropertyRef, Return };

    Value() : _value(nullptr), _type(Type::None), _id(0) { }
    Value(const Value& other) { *this = other; }
    Value(Value& other) { *this = other; }
    
    Value(Object* obj) : _value(valueFromObj(obj)) , _type(Type::Object), _id(0) { }
    Value(Float value) : _value(valueFromFloat(value)) , _type(Type::Float), _id(0) { }
    Value(int32_t value) : _value(valueFromInt(value)) , _type(Type::Integer), _id(0) { }
    Value(uint32_t value, Type type = Type::Integer) : _value(valueFromUInt(value)) , _type(type), _id(0) { }
    Value(Atom value) : _value(nullptr), _type(Type::Id), _id(value.raw()) { }
    Value(Object* obj, uint16_t index, bool property) : _value(valueFromObj(obj)), _type(property ? Type::PropertyRef : Type::ElementRef), _id(index) { }
    Value(const char* value) : _value(valueFromStr(strdup(value))) , _type(Type::String), _id(0) { }
    
    // Steals the value pointer
//    Value& operator=(Value&& other)
//    {
//        _value = other._value;
//        _type = other._type;
//        _id = other._id;
//        return *this;
//    }
    
    Value& operator=(const Value& other)
    {
        if (asStringValue()) {
            //free(static_cast<void*>(const_cast<char*>(asStringValue())));
        }
        if (other.asStringValue()) {
            _value = valueFromStr(strdup(other.asStringValue()));
        } else {
            _value = other._value;
        }
        _type = other._type;
        _id = other._id;
        return *this;
    }

    ~Value() { /*if (asStringValue()) free(static_cast<void*>(const_cast<char*>(asStringValue())));*/ }
    
    Type type() const { return _type; }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    Object* asObjectValue() const { return (_type == Type::Object) ? objFromValue() : nullptr; }
    int32_t asIntValue() const { return (_type == Type::Integer) ? intFromValue() : 0; }
    uint32_t asUIntValue() const { return (_type == Type::Integer) ? uintFromValue() : 0; }
    Float asFloatValue() const { return (_type == Type::Float) ? floatFromValue() : Float(); }
    Atom asIdValue() const { return (_type == Type::Id) ? Atom(_id) : Atom(); }

    const char* asStringValue() const { return (_type == Type::String) ? strFromValue() : nullptr; }
    
    m8r::String toStringValue() const;
    bool toBoolValue() const;
    Float toFloatValue() const;
    Object* toObjectValue() const;

    int32_t toIntValue() const { return static_cast<int32_t>(toUIntValue()); }
    uint32_t toUIntValue() const
    {
        if (_type == Type::Integer) {
            return asUIntValue();
        }
        return canBeBaked() ? bakeValue().toUIntValue() : static_cast<uint32_t>(toFloatValue());
    }

    bool setValue(const Value&);
    Value bakeValue() const;
    bool canBeBaked() const { return _type == Type::PropertyRef || _type == Type::ElementRef; }
    
    Value appendPropertyRef(const Value& value) const;
    uint32_t call(Stack<Value>& stack, uint32_t nparams);
    
    bool isInteger() const
    {
        return (_type == Type::Integer) || (canBeBaked() && bakeValue().isInteger());
    }
    bool isFloat() const
    {
        return (_type == Type::Float) || (canBeBaked() && bakeValue().isFloat());
    }
    bool isNumber() const { return isInteger() || isFloat(); }
    bool isLValue() const { return canBeBaked(); }
    bool isNone() const { return _type == Type::None; }

    static m8r::String toString(Float value);
    static m8r::String toString(int32_t value);
    static m8r::String toString(uint32_t value);
    static Float floatFromString(const char*);
    
private:
    inline void* valueFromFloat(Float f) const { U u; u.f = f; return u.v; }
    inline void* valueFromInt(int32_t i) const { U u; u.i = i; return u.v; }
    inline void* valueFromUInt(uint32_t i) const { U u; u.u = i; return u.v; }
    inline void* valueFromObj(Object* o) const { U u; u.o = o; return u.v; }
    inline void* valueFromStr(const char* s) const { U u; u.s = s; return u.v; }
    inline void* valueFromValuePtr(Value* val) const { U u; u.val = val; return u.v; }

    inline Float floatFromValue() const { U u; u.v = _value; return Float(u.f); }
    inline int32_t intFromValue() const { U u; u.v = _value; return u.i; }
    inline uint32_t uintFromValue() const { U u; u.v = _value; return u.u; }
    inline Object* objFromValue() const { U u; u.v = _value; return u.o; }
    inline const char* strFromValue() const { U u; u.v = _value; return u.s; }
    inline Value* valuePtrFromValue() const { U u; u.v = _value; return u.val; }
    
    void* _value;
    Type _type;
    uint16_t _id;
};

}
