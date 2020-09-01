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
#include "CallReturnValue.h"
#include "Error.h"
#include "GeneratedValues.h"
#include "Mallocator.h"
#include "SystemTime.h"

namespace m8rscript {

class StaticObject;
class NativeObject;
class MaterObject;
class Object;
class Function;
class ExecutionUnit;
class Program;
class Value;

using NativeFunction = m8r::CallReturnValue(*)(ExecutionUnit*, Value thisValue, uint32_t nparams);
class StringLiteral : public m8r::Id<uint32_t> { using Id::Id; };
class ConstantId : public m8r::Id<uint8_t> { using Id::Id; };


class Value {
public:    
    enum class Type : uint32_t {
        Undefined = 0,
        Float = 1,
        NativeFunction = 2,
        StaticObject = 3,
        Object = 4,
        Integer = 8,
        String = 12,
        StringLiteral = 16,
        Id = 20,
        Null = 24,
        NativeObject = 28,
        RawPointer = 32,
    };
        
    void init() { _value._type = Type::Undefined; _value._intptr = 0; }
    void copy(const Value& other) { _value._type = other._value._type; _value._intptr = other._value._intptr; }

    Value() { init(); _value._type = Type::Undefined; }
    
    explicit Value(float value) {
        _value._float = value;
        _value._type = Type::Float;
    }
    
    explicit Value(NativeFunction value)
    {
        assert(value);
        _value._type = Type::NativeFunction;
        _value._intptr = intptr_t(value);
    }

    explicit Value(StaticObject* value)
    {
        assert(value);
        _value._type = Type::StaticObject;
        _value._intptr = intptr_t(value);
    }
    
    explicit Value(void* value)
    {
        _value._type = Type::RawPointer;
        _value._intptr = intptr_t(value);
    }

    explicit Value(m8r::Mad<Object> value) { setMad(value); _value._type = Type::Object; }
    explicit Value(m8r::Mad<Function> value) { setMad(value); _value._type = Type::Object; }
    explicit Value(m8r::Mad<m8r::String> value) { setMad(value); _value._type = Type::String; }
    explicit Value(m8r::Mad<NativeObject> value) { setMad(value); _value._type = Type::NativeObject; }

    explicit Value(int32_t value) { init(); _value._int = value; _value._type = Type::Integer; }
    explicit Value(m8r::Atom value) { init(); _value._int = value.raw(); _value._type = Type::Id; }
    explicit Value(StringLiteral value) { init(); _value._int = value.raw(); _value._type = Type::StringLiteral; }
    
    // Define these to make sure no implicit functions are being called
    Value(const Value& other) { copy(other); }
    Value(Value&& other) { copy(other); }
    Value& operator=(const Value& other) { copy(other); return *this; }
    Value& operator=(Value&& other) { copy(other); return *this; }
    
    static Value NullValue() { Value value; value.init(); value._value._type = Type::Null; return value; }
    
    bool operator==(const Value& other) { return _value._type == other._value._type && _value._intptr == other._value._intptr; }
    bool operator!=(const Value& other) { return !(*this == other); }
    
    explicit operator bool() const { return type() != Type::Undefined; }

    ~Value() { }
    
    Type type() const { return _value._type; }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    m8r::Mad<Object> asObject() const { return (type() == Type::Object) ? getMad<Object>() : m8r::Mad<Object>(); }
    m8r::Mad<m8r::String> asString() const { return (type() == Type::String) ? getMad<m8r::String>() : m8r::Mad<m8r::String>(); }
    StringLiteral asStringLiteralValue() const { return (type() == Type::StringLiteral) ? stringLiteralFromValue() : StringLiteral(); }
    int32_t asIntValue() const { return (type() == Type::Integer) ? int32FromValue() : 0; }
    float asFloatValue() const { return (type() == Type::Float) ? floatFromValue() : 0; }
    m8r::Atom asIdValue() const { return (type() == Type::Id) ? atomFromValue() : m8r::Atom(); }
    m8r::Mad<NativeObject> asNativeObject() const { return (type() == Type::NativeObject) ? getMad<NativeObject>() : m8r::Mad<NativeObject>(); }
    NativeFunction asNativeFunction() { return (type() == Type::NativeFunction) ? nativeFunctionFromValue() : nullptr; }
    StaticObject* asStaticObject() { return (type() == Type::StaticObject) ? staticObjectFromValue() : nullptr; }
    const StaticObject* asStaticObject() const { return (type() == Type::StaticObject) ? staticObjectFromValue() : nullptr; }
    void* asRawPointer() const { return (type() == Type::RawPointer) ? reinterpret_cast<void*>(_value._intptr) : nullptr; }

    static Value asValue(m8r::Mad<NativeObject> obj) { return Value(static_cast<m8r::Mad<NativeObject>>(obj)); }
    
    m8r::String toStringValue(ExecutionUnit*) const;
    const char* toStringPointer(ExecutionUnit*) const;
    bool toBoolValue(ExecutionUnit* eu) const
    {
        switch (type()) {
            default:
            case Type::Null:
            case Type::Undefined:       return false;
            case Type::String:          return asString().valid() && !asString()->empty();
            case Type::Object:          return asObject().valid();  
            case Type::NativeObject:    return asNativeObject().valid();
            case Type::StaticObject:
            case Type::NativeFunction:  return _value._intptr != 0;
            case Type::Integer:         return int32FromValue() != 0;
            case Type::Float:
            case Type::StringLiteral:
            case Type::Id:              return toIntValue(eu) != 0;
        }
    }
    
    float toFloatValue(ExecutionUnit* eu) const
    {
        return (type() == Type::Float) ? floatFromValue() : ((type() == Type::Integer) ? float(int32FromValue()) : _toFloatValue(eu));
    }

    int32_t toIntValue(ExecutionUnit* eu) const
    {
        if (type() == Type::Integer) {
            return int32FromValue();
        }
        return static_cast<int32_t>(toFloatValue(eu));
    }
    
    m8r::Atom toIdValue(ExecutionUnit* eu) const
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
    bool isId() const { return type() == Type::Id; }
    bool isUndefined() const { return type() == Type::Undefined; }
    bool isObject() const { return type() == Type::Object; }
    bool isNativeObject() const { return type() == Type::NativeObject; }
    bool isNativeFunction() const { return type() == Type::NativeFunction; }
    bool isStaticObject() const { return type() == Type::StaticObject; }
    bool isPointer() const { return isObject() || isNativeObject(); }

    bool isType(ExecutionUnit*, m8r::Atom);
    bool isType(ExecutionUnit*, SA);

    void gcMark() const;
    
    enum class SetType { AlwaysAdd, NeverAdd, AddIfNeeded };

    const Value property(const m8r::Atom&) const;
    const Value property(ExecutionUnit*, const m8r::Atom&) const;
    bool setProperty(const m8r::Atom& prop, const Value& value, Value::SetType);
    const Value element(ExecutionUnit* eu, const Value& elt) const;
    bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, Value::SetType);

    m8r::CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    m8r::CallReturnValue construct(ExecutionUnit* eu, uint32_t nparams);
    m8r::CallReturnValue callProperty(ExecutionUnit*, m8r::Atom prop, uint32_t nparams);
        
    bool needsGC() const { return type() == Type::Object || type() == Type::String; }
    
private:
    float _toFloatValue(ExecutionUnit*) const;
    Value _toValue(ExecutionUnit*) const;
    m8r::Atom _toIdValue(ExecutionUnit*) const;

    inline float floatFromValue() const { return _value._float; }
    int32_t int32FromValue() const { return _value._int; }
    uint32_t uint32FromValue() const { return _value._int; }
    m8r::Atom atomFromValue() const { return m8r::Atom(static_cast<m8r::Atom::value_type>(_value._int)); }
    NativeFunction nativeFunctionFromValue() { return reinterpret_cast<NativeFunction>(_value._intptr); }
    StaticObject* staticObjectFromValue() { return reinterpret_cast<StaticObject*>(_value._intptr); }
    const StaticObject* staticObjectFromValue() const { return reinterpret_cast<StaticObject*>(_value._intptr); }

    StringLiteral stringLiteralFromValue() const
    {
        return StringLiteral(static_cast<StringLiteral::Raw>(_value._int));
    }

    template<typename T>
    void setMad(m8r::Mad<T> v) { assert(v.valid()); init(); _value._rawMad = v.raw(); }
    
    template<typename T>
    m8r::Mad<T> getMad()const { return m8r::Mad<T>(_value._rawMad); }
    
    // A value is the size of a pointer. This can contain a Float (which can be up to 
    // 64 bits), a NativeFunction or StaticObject pointer, or a structure containing 
    // a type and either an int32_t or a RawMad.
    //
    // The Type field is used for all types except NativeFunction and Float. For these the
    // lowest 2 bits are used for type. For pointers, they are 4 byte aligned on ESP and
    // 8 byte aligned on Mac. So using the lowest 2 bits is safe. For float, using the
    // lowest 2 bits means you lose 2 bits of precision.
    struct {
        Type _type;
        union {
            intptr_t _intptr;
            m8r::RawMad _rawMad;
            int32_t _int;
            float _float;
        };
    } _value;
    
    // In order to fit everything, we have some requirements
    static_assert(sizeof(_value) == (sizeof(intptr_t) * 2), "Value must be twice the size of intptr_t");
};

}
