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
using InstructionVector = FixedVector<uint8_t>;
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
    
    void operator delete(void* p);
    
    template<typename T>
    static Mad<T> create() { Mad<T> obj = Mad<T>::create(MemoryType::Object); addToObjectStore(obj.raw()); return obj; }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const;
    
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
    virtual uint16_t localCount() const { return 0; }
    virtual const ConstantValueVector*  constants() const { return nullptr; }
    virtual uint16_t formalParamCount() const { return 0; }
    virtual bool loadUpValue(ExecutionUnit*, uint32_t index, Value&) const { return false; }
    virtual bool storeUpValue(ExecutionUnit*, uint32_t index, const Value&) { return false; }
    virtual Atom name() const { return Atom(); }
    virtual bool isFunction() const { return false; }
    
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
    MaterObject() { }
    virtual ~MaterObject();

    virtual String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark() override;
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append) override;

    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override;
    virtual const Value property(const Atom& prop) const override;
    virtual bool setProperty(const Atom& prop, const Value& v, Value::SetPropertyType type = Value::SetPropertyType::AddIfNeeded) override;
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;

    uint32_t numProperties() const { return static_cast<int32_t>(_properties.size()); }
    Atom propertyKeyforIndex(uint32_t i) const { return (i < numProperties()) ? (_properties.begin() + i)->key : Atom(); }
    
private:
    PropertyMap _properties;
};

class MaterArray : public Object {
public:
    MaterArray() { }
    virtual ~MaterArray() { }

    virtual String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark() override;
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append) override;

    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override;
    virtual const Value property(const Atom& prop) const override;
    virtual bool setProperty(const Atom& prop, const Value& v, Value::SetPropertyType type = Value::SetPropertyType::AddIfNeeded) override;
    
    const Value& operator[](size_t i) const { assert(i < _array.size()); return _array[i]; };
    Value& operator[](size_t i) { assert(i < _array.size()); return _array[i]; };
    size_t size() const { return _array.size(); }
    bool empty() const { return _array.empty(); }
    void clear() { _array.clear(); }
    void resize(size_t size) { _array.resize(size); }

private:
    Vector<Value> _array;
    bool _arrayNeedsGC = false;
};

class ObjectFactory;

class NativeObject {
public:
    static MemoryType memoryType() { return MemoryType::Native; }

    void operator delete(void* p);
    
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
    
    struct StaticFunctionProperty
    {
        SA name;
        NativeFunction func;
    };

    static_assert(std::is_pod<StaticFunctionProperty>::value, "StaticFunctionProperty must be pod");

    struct StaticObjectProperty
    {
        SA name;
        StaticObject* obj;
    };

    static_assert(std::is_pod<StaticObjectProperty>::value, "StaticObjectProperty must be pod");
    
    StaticObject() { }
    
    const Value property(const Atom& name) const
    {
        return const_cast<StaticObject*>(this)->property(name);
    }

    Value property(const Atom& name)
    {
        auto it = std::find_if(_functionProperties, _functionProperties + _functionPropertiesCount, 
                               [name](const StaticFunctionProperty& p) { return name == Atom(p.name); });
        if (it != _functionProperties + _functionPropertiesCount) {
            return Value(it->func);
        }
        auto it2 = std::find_if(_objectProperties, _objectProperties + _objectPropertiesCount,
                                [name](const StaticObjectProperty& p) { return name == Atom(p.name); });
        if (it2 != _objectProperties + _objectPropertiesCount) {
            return Value(it2->obj);
        }
        auto it3 = std::find(_properties, _properties + _propertiesCount, name);
        return (it3 == _properties + _propertiesCount) ? Value() : it3->value;
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

}
