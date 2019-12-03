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
#include "Value.h"
#include <algorithm>

namespace m8r {

class Error;
class ExecutionUnit;
class Object;
class Stream;

using ConstantValue = Value;
using ConstantValueVector = FixedVector<ConstantValue>;
using InstructionVector = FixedVector<Instruction>;
using PropertyMap = Map<Atom, Value>;

class Object {    
public:
    Object()
        :  _marked(true)
        , _hasGet(false)
        , _hasSet(false)
        , _isDestroyed(false)
    { }
    virtual ~Object() { _isDestroyed = true; }
    
    void operator delete(void* p, std::size_t sz);
    
    template<typename T>
    static Mad<T> create() { Mad<T> obj = Mad<T>::create(MemoryType::Object); addToObjectStore(obj.raw()); return obj; }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const { return typeOnly ? String() : toString(eu, true) + " { }"; }
    
    virtual void gcMark() { gcMark(this); _proto.gcMark(); }
    
    virtual const Value property(const Atom&) const { return Value(); }
    
    virtual bool setProperty(const Atom& prop, const Value& value, Value::SetPropertyType = Value::SetPropertyType::AddIfNeeded) { return false; }
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const { return property(elt.toIdValue(eu)); }
    
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
    {
        return setProperty(elt.toIdValue(eu), value, append ? Value::SetPropertyType::AlwaysAdd : Value::SetPropertyType::NeverAdd);
    }

    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) { return CallReturnValue(CallReturnValue::Error::Unimplemented); }
    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) { return CallReturnValue(CallReturnValue::Error::Unimplemented); }
    
    void setMarked(bool b) { _marked = b; }
    bool isMarked() const { return _marked; }

    bool hasGet() const { return _hasGet; }
    bool hasSet() const { return _hasSet; }
    
    // methods for Callable (m8rscript) objects
    virtual const InstructionVector* code() const { return nullptr; }
    virtual uint32_t localSize() const { return 0; }
    virtual const ConstantValueVector*  constants() const { return nullptr; }
    virtual uint32_t formalParamCount() const { return 0; }
    virtual bool loadUpValue(ExecutionUnit*, uint32_t index, Value&) const { return false; }
    virtual bool storeUpValue(ExecutionUnit*, uint32_t index, const Value&) { return false; }
    virtual Atom name() const { return Atom(); }
    virtual bool hasUpValues() const { return false; }
    virtual bool isFunction() const { return false; }
    virtual void setArray(bool b) { }
    
    static void gcMark(Object* obj) { if (obj) obj->setMarked(true); }
    
    Atom typeName() const { return _typeName; }
    void setTypeName(Atom name) { _typeName = name; }
    
    static CallReturnValue construct(const Value& proto, ExecutionUnit*, uint32_t nparams);

protected:
    void setProto(const Value& val) { _proto = val; }
    Value proto() const { return _proto; }
    
    void setHasGet(bool b) { _hasGet = b; }
    void setHasSet(bool b) { _hasSet = b; }
    
    static void addToObjectStore(RawMad);
    
private:
    Value _proto;
    bool _marked : 1;
    bool _hasGet : 1;
    bool _hasSet : 1;
    bool _isDestroyed : 1;
    Atom _typeName;
};

class MaterObject : public Object {
public:
    MaterObject(bool isArray = false) : _isArray(isArray) { }
    virtual ~MaterObject();

    virtual String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark() override;
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append) override;

    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override;
    virtual const Value property(const Atom& prop) const override;
    virtual bool setProperty(const Atom& prop, const Value& v, Value::SetPropertyType type = Value::SetPropertyType::AddIfNeeded) override;
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;

    virtual void setArray(bool b) override { _isArray = b; }

    uint32_t numProperties() const { return static_cast<int32_t>(_properties.size()); }
    Atom propertyKeyforIndex(uint32_t i) const { return (i < numProperties()) ? (_properties.begin() + i)->key : Atom(); }
    
    const Value& operator[](size_t i) const { assert(i >= 0 && i < _array.size()); return _array[i]; };
    Value& operator[](size_t i) { assert(i >= 0 && i < _array.size()); return _array[i]; };
	size_t size() const { return _array.size(); }
    bool empty() const { return _array.empty(); }
    void clear() { _array.clear(); }
    void resize(size_t size) { _array.resize(size); }

private:
    PropertyMap _properties;
    Vector<Value> _array;
    bool _isArray = false;
    bool _arrayNeedsGC = false;
};

class ObjectFactory;

class NativeObject {
public:
    static MemoryType memoryType() { return MemoryType::Native; }

    void operator delete(void* p, std::size_t sz);
    
    NativeObject() { }
    virtual ~NativeObject() { }

    virtual void gcMark() { }
};

template<typename T>
CallReturnValue getNative(Mad<T>& nativeObj, ExecutionUnit* eu, Value thisValue)
{
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    nativeObj = obj->property(Atom(SA::__nativeObject)).asNativeObject();
    if (!nativeObj.valid()) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }
    return CallReturnValue(CallReturnValue::Error::Ok);
}

class ObjectFactory {
public:    
    ObjectFactory(SA, ObjectFactory* parent = nullptr, NativeFunction constructor = nullptr);
    ~ObjectFactory();
    
    void addProperty(Atom prop, Mad<Object>);
    void addProperty(Atom prop, const Value&);
    void addProperty(SA, Mad<Object>);
    void addProperty(SA, const Value&);
    void addProperty(SA, NativeFunction);

    Mad<Object> nativeObject() { return _obj; }
    const Mad<const Object> nativeObject() const { return _obj; }
    
    static Mad<Object> create(Atom objectName, ExecutionUnit*, uint32_t nparams);

protected:
    Mad<MaterObject> _obj;
    NativeFunction _constructor;
};

class StaticObject
{
public:
    struct StaticProperty
    {
        bool operator==(const Atom& atom) const { return name == atom; }
        Atom name;
        Value value;
    };
    
    StaticObject() { }
    
    const Value property(const Atom& name) const
    {
        auto it = std::find(_properties, _properties + _propertiesCount, name);
        return (it == _properties + _propertiesCount) ? Value() : it->value;
    }

    Value property(const Atom& name)
    {
        auto it = std::find(_properties, _properties + _propertiesCount, name);
        return (it == _properties + _propertiesCount) ? Value() : it->value;
    }
    
    void setProperties(StaticProperty* props, size_t count)
    {
        _properties = props;
        _propertiesCount = count;
    }

protected:
    const StaticProperty* _properties = nullptr;
    uint16_t _propertiesCount = 0;
};

}
