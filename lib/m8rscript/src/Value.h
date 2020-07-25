/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Defines.h"
#if M8RSCRIPT_SUPPORT == 1

#include "Atom.h"
#include "CallReturnValue.h"
#include "Error.h"
#include "Mallocator.h"
#include "SystemTime.h"

namespace m8r {

class StaticObject;
class NativeObject;
class MaterObject;
class Object;
class Function;
class ExecutionUnit;
class Program;
class Value;

using NativeFunction = CallReturnValue(*)(ExecutionUnit*, Value thisValue, uint32_t nparams);

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
    };
    
    static constexpr auto ValueMask = 0x03;
    
    void init() { memset(_value._raw, 0, sizeof(_value._raw)); }
    void copy(const Value& other) { memcpy(_value._raw, other._value._raw, sizeof(_value._raw)); }

    Value() { init(); _value._type = Type::Undefined; }
    
    explicit Value(Float value) {
        _value._float = value.raw() & ~ValueMask;
        _value._float |= static_cast<Float::decompose_type>(Type::Float);
    }
    
    explicit Value(NativeFunction value)
    {
        assert(value);
        assert((reinterpret_cast<intptr_t>(value) & ValueMask) == 0);
        _value._intptr = reinterpret_cast<intptr_t>(value) | static_cast<intptr_t>(Type::NativeFunction);
    }

    explicit Value(StaticObject* value)
    {
        assert(value);
        assert((reinterpret_cast<intptr_t>(value) & ValueMask) == 0);
        _value._intptr = reinterpret_cast<intptr_t>(value) | static_cast<intptr_t>(Type::StaticObject);
    }

    explicit Value(Mad<Object> value) { setMad(value); _value._type = Type::Object; }
    explicit Value(Mad<Function> value) { setMad(value); _value._type = Type::Object; }
    explicit Value(Mad<String> value) { setMad(value); _value._type = Type::String; }
    explicit Value(Mad<NativeObject> value) { setMad(value); _value._type = Type::NativeObject; }

    explicit Value(int32_t value) { init(); _value._int = value; _value._type = Type::Integer; }
    explicit Value(Atom value) { init(); _value._int = value.raw(); _value._type = Type::Id; }
    explicit Value(StringLiteral value) { init(); _value._int = value.raw(); _value._type = Type::StringLiteral; }
    
    // Define these to make sure no implicit functions are being called
    Value(const Value& other) { copy(other); }
    Value(Value&& other) { copy(other); }
    Value& operator=(const Value& other) { copy(other); return *this; }
    Value& operator=(Value&& other) { copy(other); return *this; }
    
    static Value NullValue() { Value value; value.init(); value._value._type = Type::Null; return value; }
    
    bool operator==(const Value& other) { return _value._raw == other._value._raw; }
    bool operator!=(const Value& other) { return _value._raw != other._value._raw; }
    
    explicit operator bool() const { return type() != Type::Undefined; }

    ~Value() { }
    
    Type type() const { return ((_value._intptr & ValueMask) == 0) ? _value._type : static_cast<Type>(_value._float & ValueMask); }
    
    //
    // asXXX() functions are lightweight and simply cast the Value to that type. If not the correct type it returns 0 or null
    // toXXX() functions are heavyweight and attempt to convert the Value type to a primitive of the requested type
    
    Mad<Object> asObject() const { return (type() == Type::Object) ? getMad<Object>() : Mad<Object>(); }
    Mad<String> asString() const { return (type() == Type::String) ? getMad<String>() : Mad<String>(); }
    StringLiteral asStringLiteralValue() const { return (type() == Type::StringLiteral) ? stringLiteralFromValue() : StringLiteral(); }
    int32_t asIntValue() const { return (type() == Type::Integer) ? int32FromValue() : 0; }
    Float asFloatValue() const { return (type() == Type::Float) ? floatFromValue() : Float(); }
    Atom asIdValue() const { return (type() == Type::Id) ? atomFromValue() : Atom(); }
    Mad<NativeObject> asNativeObject() const { return (type() == Type::NativeObject) ? getMad<NativeObject>() : Mad<NativeObject>(); }
    NativeFunction asNativeFunction() { return (type() == Type::NativeFunction) ? nativeFunctionFromValue() : nullptr; }
    StaticObject* asStaticObject() { return (type() == Type::StaticObject) ? staticObjectFromValue() : nullptr; }
    const StaticObject* asStaticObject() const { return (type() == Type::StaticObject) ? staticObjectFromValue() : nullptr; }

    static Value asValue(Mad<NativeObject> obj) { return Value(static_cast<Mad<NativeObject>>(obj)); }
    
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
    bool isId() const { return type() == Type::Id; }
    bool isUndefined() const { return type() == Type::Undefined; }
    bool isObject() const { return type() == Type::Object; }
    bool isNativeObject() const { return type() == Type::NativeObject; }
    bool isNativeFunction() const { return type() == Type::NativeFunction; }
    bool isStaticObject() const { return type() == Type::StaticObject; }
    bool isPointer() const { return isObject() || isNativeObject(); }

    bool isType(ExecutionUnit*, Atom);
    bool isType(ExecutionUnit*, SA);

    void gcMark();
    
    enum class SetType { AlwaysAdd, NeverAdd, AddIfNeeded };

    const Value property(const Atom&) const;
    const Value property(ExecutionUnit*, const Atom&) const;
    bool setProperty(ExecutionUnit*, const Atom& prop, const Value& value, Value::SetType);
    const Value element(ExecutionUnit* eu, const Value& elt) const;
    bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, Value::SetType);

    CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    CallReturnValue construct(ExecutionUnit* eu, uint32_t nparams);
    CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams);
        
    bool needsGC() const { return type() == Type::Object || type() == Type::String; }
    
private:
    Float _toFloatValue(ExecutionUnit*) const;
    Value _toValue(ExecutionUnit*) const;
    Atom _toIdValue(ExecutionUnit*) const;

    inline Float floatFromValue() const
    {
        Float::Raw raw(_value._float & ~3);
        return Float(raw);
    }
    int32_t int32FromValue() const { return _value._int; }
    uint32_t uint32FromValue() const { return _value._int; }
    Atom atomFromValue() const { return Atom(static_cast<Atom::value_type>(_value._int)); }
    NativeFunction nativeFunctionFromValue() { return reinterpret_cast<NativeFunction>(_value._intptr & ~ValueMask); }
    StaticObject* staticObjectFromValue() { return reinterpret_cast<StaticObject*>(_value._intptr & ~ValueMask); }
    const StaticObject* staticObjectFromValue() const { return reinterpret_cast<StaticObject*>(_value._intptr & ~ValueMask); }

    StringLiteral stringLiteralFromValue() const
    {
        return StringLiteral(static_cast<StringLiteral::Raw>(_value._int));
    }

    template<typename T>
    void setMad(Mad<T> v) { assert(v.valid()); init(); _value._rawMad = v.raw(); }
    
    template<typename T>
    Mad<T> getMad()const { return Mad<T>(_value._rawMad); }
    
    // A value is the size of a pointer. This can contain a Float (which can be up to 
    // 64 bits), a NativeFunction or StaticObject pointer, or a structure containing 
    // a type and either an int32_t or a RawMad.
    //
    // The Type field is used for all types except NativeFunction and Float. For these the
    // lowest 2 bits are used for type. For pointers, they are 4 byte aligned on ESP and
    // 8 byte aligned on Mac. So using the lowest 2 bits is safe. For float, using the
    // lowest 2 bits means you lose 2 bits of precision.
    union {
        uint8_t _raw[sizeof(intptr_t) * 2];
        Float::value_type _float;
        intptr_t _intptr;
        struct {
            // TODO: type needs to be in the low order bytes of Value
            // This makes the trick of using the 2 LSB as the type forFloat and
            // NativeFunction. That's only true for little endian platforms. Both
            // Mac and ESP are little endian, but if it's ever ported big endian this
            // needs to be fixed
            Type _type;
            union {
                RawMad _rawMad;
                int32_t _int;
            };
        };
    } _value;
    
    // In order to fit everything, we have some requirements
    static_assert(sizeof(_value) == (sizeof(intptr_t) * 2), "Value must be twice the size of intptr_t");
};

}

#endif
