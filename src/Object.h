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
class Program;
class Stream;

using ConstantValue = Value;
using ConstantValueVector = std::vector<ConstantValue>;
using InstructionVector = std::vector<Instruction>;

class Object {    
public:
    Object()
        :  _marked(true)
        , _hasIterator(false)
        , _hasGet(false)
        , _hasSet(false)
    { }
    virtual ~Object() { }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const { return typeOnly ? String() : toString(eu, true) + " { }"; }
    
    virtual void gcMark() { gcMark(this); gcMark(_proto);; }
    
    virtual const Value property(ExecutionUnit*, const Atom&) const { return Value(); }
    
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& value, Value::SetPropertyType) { return false; }
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const { return property(eu, elt.toIdValue(eu)); }
    
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
    {
        return setProperty(eu, elt.toIdValue(eu), value, append ? Value::SetPropertyType::AlwaysAdd : Value::SetPropertyType::NeverAdd);
    }

    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) { return CallReturnValue(CallReturnValue::Error::Unimplemented); }
    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) { return CallReturnValue(CallReturnValue::Error::Unimplemented); }
    
    void setMarked(bool b) { _marked = b; }
    bool isMarked() const { return _marked; }

    bool hasIterator() const { return _hasIterator; }
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
    
    static void addStaticObject(Mad<Object> obj) { _staticObjects.push_back(obj); }
    static void removeStaticObject(Mad<Object> obj)
    {
        auto it = std::find(_staticObjects.begin(), _staticObjects.end(), obj);
        if (it != _staticObjects.end()) {
            _staticObjects.erase(it);
        }
    }

    static void addEU(Mad<ExecutionUnit> eu) { _euStore.push_back(eu); }
    static void removeEU(Mad<ExecutionUnit> eu)
    {
        auto it = std::find(_euStore.begin(), _euStore.end(), eu);
        if (it != _euStore.end()) {
            _euStore.erase(it);
        }
    }

    static void gc(bool force = false);
    static void gcMark(Object* obj) { if (obj) obj->setMarked(true); }
    
    static String* createString() { String* string = new String(); _stringStore.push_back(string); return string; }
    static String* createString(const char* s, int32_t length = -1)  { String* string = new String(s, length); _stringStore.push_back(string); return string; }
    static String* createString(const String& s)  { String* string = new String(s); _stringStore.push_back(string); return string; }
    static uint32_t numObjectAllocations() { return static_cast<uint32_t>(_objectStore.size()); }
    static uint32_t numStringAllocations() { return static_cast<uint32_t>(_stringStore.size()); }
    
    static void memoryInfo(MemoryInfo&);
    
    Atom typeName(ExecutionUnit*) const;
    
protected:
    void setProto(Object* obj) { _proto = obj; }
    Object* proto() const { return _proto; }
    
    void setHasIterator(bool b) { _hasIterator = b; }
    void setHasGet(bool b) { _hasGet = b; }
    void setHasSet(bool b) { _hasSet = b; }
    
private:
    Object* _proto = nullptr;
    bool _marked : 1;
    bool _hasIterator : 1;
    bool _hasGet : 1;
    bool _hasSet : 1;

    static std::vector<Mad<String>> _stringStore;
    static std::vector<Mad<Object>> _objectStore;
    static std::vector<Mad<Object>> _staticObjects;
    static std::vector<Mad<ExecutionUnit>> _euStore;
};

template<> inline MemoryType Mad<Object>::type()        { return MemoryType::Object; }
template<> inline MemoryType Mad<ConstantValue>::type() { return MemoryType::ConstantValue; }

class MaterObject : public Object {
public:
    MaterObject(bool isArray = false) : _isArray(isArray) { }
    virtual ~MaterObject();

    virtual String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark() override;
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append) override;

    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override;
    virtual const Value property(ExecutionUnit* eu, const Atom& prop) const override;
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& v, Value::SetPropertyType type) override;
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;

    virtual void setArray(bool b) override { _isArray = b; }

    uint32_t numProperties() const { return static_cast<int32_t>(_properties.size()); }
    Atom propertyKeyforIndex(uint32_t i) const { return (i < numProperties()) ? (_properties.begin() + i)->key : Atom(); }
    
    bool setProperty(const Atom& prop, const Value& v);

    const Value& operator[](size_t i) const { assert(i >= 0 && i < _array.size()); return _array[i]; };
    Value& operator[](size_t i) { assert(i >= 0 && i < _array.size()); return _array[i]; };
	size_t size() const { return _array.size(); }
    bool empty() const { return _array.empty(); }
    void clear() { _array.clear(); }
    void resize(size_t size) { _array.resize(size); }

private:
    Value::Map _properties;
    std::vector<Value> _array;
    Mad<Object> _iterator;
    NativeObject* _nativeObject = nullptr;
    bool _isArray = false;
    bool _arrayNeedsGC;
};

class ObjectFactory;

class NativeObject {
public:
    NativeObject() { }
    virtual ~NativeObject() { }

    virtual void gcMark() { }
};

template<typename T>
CallReturnValue getNative(T*& nativeObj, ExecutionUnit* eu, Value thisValue)
{
    Mad<Object> obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    nativeObj = static_cast<T*>(obj->property(eu, Atom(SA::__nativeObject)).asNativeObject());
    if (!nativeObj) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }
    return CallReturnValue(CallReturnValue::Error::Ok);
}

class ObjectFactory {
public:
    ObjectFactory(Mad<Program>, SA, ObjectFactory* parent = nullptr, NativeFunction constructor = nullptr);
    ~ObjectFactory();
    
    void addProperty(Atom prop, Mad<Object>);
    void addProperty(Atom prop, const Value&);
    void addProperty(Mad<Program>, SA, Mad<Object>);
    void addProperty(Mad<Program>, SA, const Value&);
    void addProperty(Mad<Program>, SA, NativeFunction);

    Mad<Object> nativeObject() { return _obj; }
    const Mad<const Object> nativeObject() const { return _obj; }
    
    static Mad<Object> create(Atom objectName, ExecutionUnit*, uint32_t nparams);
    static Mad<Object> create(Mad<Object> proto, ExecutionUnit*, uint32_t nparams);

protected:
    Mad<MaterObject> _obj;
    NativeFunction _constructor;
};
    
}
