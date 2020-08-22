/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "SharedPtr.h"

namespace marly {

class Value;

using ValueMap = m8r::Map<m8r::Atom, Value>;
using ValueVector = m8r::Vector<Value>;

class ObjectBase : public m8r::Shared
{
public:
    virtual ~ObjectBase() { }
    virtual Value property(m8r::Atom) const;
    virtual void setProperty(m8r::Atom, const Value&) { }
};

class Object : public ObjectBase, public ValueMap
{
public:
    virtual ~Object() { }
    virtual Value property(m8r::Atom) const override;
    virtual void setProperty(m8r::Atom, const Value&) override { }
};

class List : public ObjectBase, public ValueVector
{
public:
    virtual ~List() { }
    virtual Value property(m8r::Atom) const override;
    virtual void setProperty(m8r::Atom prop, const Value& value) override;
};

class String : public ObjectBase
{
public:
    virtual ~String() { }
    
    m8r::String& string() { return _str; }
    const m8r::String& string() const { return _str; }

    virtual Value property(m8r::Atom) const override;
    virtual void setProperty(m8r::Atom, const Value&) override { }

private:
    m8r::String _str;
};

class Value
{
public:
    enum class Type : uint16_t {
        // Built-in verbs are first so they can
        // have the same values as the shared atoms
        Verb = m8r::ExternalAtomOffset,
        Bool, Null, Undefined, 
        Int, Float,
        String, List, Object,
        
        // Built-in operators
        Load, Store, LoadProp, StoreProp,
        TokenVerb,
    };
    
    Value() { _type = Type::Undefined; _int = 0; }
    Value(bool b) { _type = Type::Bool; _bool = b; }
    Value(Type t) { _type = t; }
    Value(const char* s)
    {
        _type = Type::String;
        _ptr = nullptr;
        m8r::SharedPtr<String>* str = reinterpret_cast<m8r::SharedPtr<String>*>(&_ptr);
        str->reset(new String());
        (*str)->string() = s;
    }
    
    Value(float f) { _type = Type::Float; _float = f; }
    Value(const m8r::SharedPtr<List>& list) { setValue(Type::List, list.get()); }
    Value(List* list) { setValue(Type::List, list); }
    Value(const m8r::SharedPtr<String>& string) { setValue(Type::String, string.get()); }
    Value(String* string) { setValue(Type::String, string); }
    Value(const m8r::SharedPtr<Object>& object) { setValue(Type::Object, object.get()); }
    Value(Object* object) { setValue(Type::Object, object); }
    
    Value(int32_t i, Type type = Type::Int)
    {
        _type = type;
        switch(type) {
            case Type::Bool: _bool = i != 0; break;
            case Type::Verb:
            case Type::Float: _float = i; break;
            case Type::Int:
            default: _int = i; 
        }
    }
    
    Type type() const {return _type; }
    
    bool isBuiltInVerb() const { return int(_type) < m8r::ExternalAtomOffset; }
    SA builtInVerb() const { assert(isBuiltInVerb()); return static_cast<SA>(_type); }
    
    m8r::SharedPtr<String> string() const
    {
        assert(_type == Type::String);
        return m8r::SharedPtr<String>(reinterpret_cast<String*>(_ptr));
    }
    
    m8r::SharedPtr<List> list() const
    {
        assert(_type == Type::List);
        return m8r::SharedPtr<List>(reinterpret_cast<List*>(_ptr));
    }
    
    m8r::SharedPtr<Object> object() const
    {
        assert(_type == Type::Object);
        return m8r::SharedPtr<Object>(reinterpret_cast<Object*>(_ptr));
    }
    
    // FIXME: We need to handle all types here
    int32_t integer() const
    {
        switch(_type) {
            case Type::String: return string()->string().toInt();
            case Type::Bool: return _bool ? 1 : 0;
            case Type::Int: return _int;
            case Type::Float: return(int32_t(_float));
            case Type::List:
            case Type::Object: return 0;

            // For all other types we assume the value stored is an int
            default: return _int;
        }
    }
    
    float flt() const
    {
        switch(_type) {
            // FIXME: Do a toFloat conversion
            case Type::String: return string()->string().toFloat();
            case Type::Bool: return _bool ? 1 : 0;
            case Type::Int: return _int;
            case Type::Float: return _float;
            default: return 0;
        }
    }

    bool boolean() const
    {
        switch(_type) {
            // FIXME: Do a toFloat conversion
            case Type::String: return string()->string().toInt() != 0;
            case Type::Bool: return _bool;
            case Type::Int: return _int != 0;
            case Type::Float: return _float != 0;
            default: return false;
        }
    }
    
    void toString(String& str) const
    {
        switch(_type) {
            case Type::String: str.string() = string()->string(); return;
            case Type::Bool: str.string() = _bool ? "true" : "false"; return;
            case Type::Int: str.string() = m8r::String(_int); return;
            case Type::Float: str.string() = m8r::String(flt()); return;
            default: str.string() = "** unimplemented **";
        }
    }

    Value property(m8r::Atom prop) const
    {
        switch(_type) {
            case Type::List:
            case Type::String:
            case Type::Object:
                assert(_ptr);
                return reinterpret_cast<ObjectBase*>(_ptr)->property(prop);
            default:
                return Value();
        }
    }
    
    void setProperty(m8r::Atom prop, const Value& val)
    {
        switch(_type) {
            case Type::List:
            case Type::String:
            case Type::Object:
                assert(_ptr);
                reinterpret_cast<ObjectBase*>(_ptr)->setProperty(prop, val);
            default:
                return;
        }
    }
    
    void push_back(const Value& value)
    {
        if (_type == Type::List) {
            list()->push_back(value);
        }
    }

private:
    void setValue(Type type, ObjectBase* obj)
    {
        _type = type;
        _ptr = nullptr;
        m8r::SharedPtr<ObjectBase>* objptr = reinterpret_cast<m8r::SharedPtr<ObjectBase>*>(&_ptr);
        objptr->reset(obj);
    }
    
    Type _type;
    union {
        bool _bool;
        int32_t _int;
        float _float;
        void* _ptr;
    };
};

inline Value ObjectBase::property(m8r::Atom) const { return Value(); }
inline Value Object::property(m8r::Atom) const { return Value(); }
inline Value List::property(m8r::Atom) const { return Value(); }
inline Value String::property(m8r::Atom) const { return Value(); }
inline void List::setProperty(m8r::Atom prop, const Value& value)
{
    if (prop == m8r::Atom(static_cast<m8r::Atom::value_type>(SA::length))) {
        resize(value.integer());
    }
}

}
