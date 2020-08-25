/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Mallocator.h"
#include "Defines.h"
#include "GeneratedValues.h"
#include "SharedPtr.h"
#include "Value.h"
#include <algorithm>
#include <memory>

namespace m8rscript {

class ExecutionUnit;
class Object;

using InstructionVector = m8r::Vector<uint8_t>;
using PropertyMap = m8r::Map<m8r::Atom, Value>;

class Callable {
public:
    virtual m8r::CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams)
    {
        return m8r::CallReturnValue(m8r::Error::Code::Unimplemented);
    }
    virtual const InstructionVector* code() const { return nullptr; }
    virtual uint16_t localCount() const { return 0; }
    virtual bool constant(uint8_t reg, Value&) const { return false; }
    virtual uint16_t formalParamCount() const { return 0; }
    virtual bool loadUpValue(ExecutionUnit*, uint32_t index, Value&) const { return false; }
    virtual uint32_t upValueCount() const { return 0; }
    virtual bool upValue(uint32_t i, uint32_t& index, uint16_t& frame, m8r::Atom& name) const { return false; }

    virtual m8r::Atom name() const { return m8r::Atom(); }
};

class Object : public Callable {    
public:
    Object()
        :  _marked(true)
        , _isDestroyed(false)
    { }
    virtual ~Object() { _isDestroyed = true; }
    
    static m8r::MemoryType memoryType() { return m8r::MemoryType::Object; }
    
    template<typename T>
    static m8r::Mad<T> create() { m8r::Mad<T> obj = m8r::Mad<T>::create(m8r::MemoryType::Object); addToObjectStore(obj.raw()); return obj; }

    virtual m8r::String toString(ExecutionUnit* eu, bool typeOnly = false) const;
    
    virtual Value value(ExecutionUnit* eu) const;
    
    virtual void gcMark() { gcMark(this); _proto.gcMark(); }
    
    virtual const Value property(const m8r::Atom&) const { return Value(); }
    
    virtual bool setProperty(const m8r::Atom& prop, const Value& value, Value::Value::SetType = Value::Value::SetType::AddIfNeeded) { return false; }
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const { return property(elt.toIdValue(eu)); }
    
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, Value::SetType type)
    {
        return setProperty(elt.toIdValue(eu), value, type);
    }

    virtual uint32_t numProperties() const { return 0; }
    virtual m8r::Atom propertyKeyforIndex(uint32_t i) const { return m8r::Atom(); }
    
    virtual m8r::CallReturnValue callProperty(ExecutionUnit*, m8r::Atom prop, uint32_t nparams) { return m8r::CallReturnValue(m8r::Error::Code::Unimplemented); }
    
    void setMarked(bool b) { _marked = b; }
    bool isMarked() const { return _marked; }

    static void gcMark(Object* obj) { if (obj) obj->setMarked(true); }
    
    m8r::Atom typeName() const { return _typeName; }
    void setTypeName(m8r::Atom name) { _typeName = name; }
    
    template<typename T>
    void setNativeObject(const m8r::SharedPtr<T> obj) { _nativeObject = std::static_pointer_cast<NativeObject>(obj); }

    template<typename T>
    m8r::SharedPtr<T> nativeObject() const { return std::static_pointer_cast<T>(_nativeObject); }

    static m8r::CallReturnValue construct(const Value& proto, ExecutionUnit*, uint32_t nparams);

    template<typename T>
    m8r::SharedPtr<T> impl() const
    {
        return m8r::SharedPtr<T>(reinterpret_cast<T*>(property(SAtom(SA::__impl)).asRawPointer()));
    }
    
    virtual bool canMakeClosure() const { return false; }

protected:
    void setProto(const Value& val) { _proto = val; }
    Value proto() const { return _proto; }
    
    static void addToObjectStore(m8r::RawMad);
    
private:
    Value _proto;
    bool _marked : 1;
    bool _isDestroyed : 1;
    m8r::Atom _typeName;
    m8r::SharedPtr<NativeObject> _nativeObject;
};

class MaterObject : public Object {
public:
    MaterObject() { }
    virtual ~MaterObject();

    virtual m8r::String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark() override;
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, Value::SetType) override;

    virtual m8r::CallReturnValue callProperty(ExecutionUnit* eu, m8r::Atom prop, uint32_t nparams) override;
    virtual const Value property(const m8r::Atom& prop) const override;
    virtual bool setProperty(const m8r::Atom& prop, const Value& v, Value::Value::SetType type = Value::Value::SetType::AddIfNeeded) override;

    virtual uint32_t numProperties() const override { return static_cast<int32_t>(_properties.size()); }
    virtual m8r::Atom propertyKeyforIndex(uint32_t i) const override { return (i < numProperties()) ? (_properties.begin() + i)->key : m8r::Atom(); }

private:
    PropertyMap _properties;
};

class MaterArray : public Object {
public:
    MaterArray() { }
    virtual ~MaterArray() { }

    virtual m8r::String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark() override;
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, Value::SetType) override;

    virtual m8r::CallReturnValue callProperty(ExecutionUnit* eu, m8r::Atom prop, uint32_t nparams) override;
    virtual const Value property(const m8r::Atom& prop) const override;
    virtual bool setProperty(const m8r::Atom& prop, const Value& v, Value::Value::SetType type = Value::Value::SetType::AddIfNeeded) override;
    
    size_t size() const { return _array.size(); }
    bool empty() const { return _array.empty(); }
    void clear() { _array.clear(); }
    void resize(size_t size) { _array.resize(size); }

private:
    m8r::Vector<Value> _array;
    bool _arrayNeedsGC = false;
};

class ObjectFactory;

class ObjectFactory {
public:    
    ObjectFactory(SA, ObjectFactory* parent = nullptr, NativeFunction constructor = nullptr);
    ~ObjectFactory();
    
    void addProperty(m8r::Atom prop, m8r::Mad<Object>);
    void addProperty(m8r::Atom prop, const Value&);
    void addProperty(SA, m8r::Mad<Object>);
    void addProperty(SA, const Value&);
    void addProperty(SA, NativeFunction);

    m8r::Mad<Object> nativeObject() { return _obj; }
    const m8r::Mad<const Object> nativeObject() const { return _obj; }
    
    static m8r::Mad<Object> create(m8r::Atom objectName, ExecutionUnit*, uint32_t nparams);

protected:
    m8r::Mad<MaterObject> _obj;
    NativeFunction _constructor;
};

class StaticObject
{
public:
    struct StaticProperty
    {
        SA name() const { return _name; }
        Value value() const { return _value; }

        bool operator==(const m8r::Atom& atom) const { return SAtom(name()) == atom; }
        SA _name;
        Value _value;
    };
    
    struct StaticFunctionProperty
    {
        SA name() const { return _name; }
        NativeFunction func() const { return _func; }
        
        SA _name;
        NativeFunction _func;
    };

    static_assert(std::is_pod<StaticFunctionProperty>::value, "StaticFunctionProperty must be pod");

    struct StaticObjectProperty
    {
        SA name() const { return _name; }
        StaticObject* obj() const { return _obj; }
        
        SA _name;
        StaticObject* _obj;
    };

    static_assert(std::is_pod<StaticObjectProperty>::value, "StaticObjectProperty must be pod");
    
    StaticObject() { }
    
    const Value property(const m8r::Atom& name) const
    {
        return const_cast<StaticObject*>(this)->property(name);
    }

    Value property(const m8r::Atom& name)
    {
        auto it = std::find_if(_functionProperties, _functionProperties + _functionPropertiesCount, 
                               [name](const StaticFunctionProperty& p) { return name == SAtom(p.name()); });
        if (it != _functionProperties + _functionPropertiesCount) {
            return Value(it->func());
        }
        auto it2 = std::find_if(_objectProperties, _objectProperties + _objectPropertiesCount,
                                [name](const StaticObjectProperty& p) { return name == SAtom(p.name()); });
        if (it2 != _objectProperties + _objectPropertiesCount) {
            return Value(it2->obj());
        }
        auto it3 = std::find(_properties, _properties + _propertiesCount, name);
        return (it3 == _properties + _propertiesCount) ? Value() : it3->value();
    }
    
    void setProperties(StaticProperty* props, size_t count)
    {
        _properties = props;
        _propertiesCount = count;
    }

    void setProperties(StaticFunctionProperty* props, size_t count)
    {
        _functionProperties = props;
        _functionPropertiesCount = count;
    }

    void setProperties(StaticObjectProperty* props, size_t count)
    {
        _objectProperties = props;
        _objectPropertiesCount = count;
    }

protected:
    const StaticFunctionProperty* _functionProperties = nullptr;
    const StaticObjectProperty* _objectProperties = nullptr;
    const StaticProperty* _properties = nullptr;
    uint16_t _functionPropertiesCount = 0;
    uint16_t _objectPropertiesCount = 0;
    uint16_t _propertiesCount = 0;
};

class NativeObject : public m8r::Shared {
public:
    static m8r::MemoryType memoryType() { return m8r::MemoryType::Native; }
    
    NativeObject() { }
    virtual ~NativeObject() { }

    virtual void gcMark() { }
};

}
