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

#include "Containers.h"
#include "Defines.h"
#include "Value.h"
#include <algorithm>

namespace m8r {

class Error;
class ExecutionUnit;
class Object;
class Program;
class Stream;

class Object {    
public:
    Object()
        :  _marked(true)
        , _hasIterator(false)
        , _hasGet(false)
        , _hasSet(false)
    { }
    virtual ~Object() { }

    void* operator new(size_t size);
    void operator delete(void* p);

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const { return typeOnly ? String() : toString(eu, true) + " { }"; }
    
    virtual void gcMark(ExecutionUnit* eu) { _gcMark(eu); }
    
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
    virtual const Code* code() const { return nullptr; }
    virtual uint32_t localSize() const { return 0; }
    virtual const std::vector<Value>*  constants() const { return nullptr; }
    virtual uint32_t formalParamCount() const { return 0; }
    virtual bool loadUpValue(ExecutionUnit*, uint32_t index, Value&) const { return false; }
    virtual bool storeUpValue(ExecutionUnit*, uint32_t index, const Value&) { return false; }
    virtual Atom name() const { return Atom(); }
    virtual bool hasUpValues() const { return false; }
    virtual bool isFunction() const { return false; }
    
    static void addStaticObject(Object* obj) { _staticObjects.push_back(obj); }
    static void removeStaticObject(Object* obj)
    {
        for (auto it = _staticObjects.begin(); it != _staticObjects.end(); ++it) {
            if (*it == obj) {
                _staticObjects.erase(it);
                return;
            }
        }
    }

    static void gc(ExecutionUnit*, bool force);
    static void gcMark(Object* obj) { if (obj) obj->setMarked(true); }
    
    static String* createString() { String* string = new String(); _stringStore.push_back(string); return string; }
    static String* createString(const char* s, int32_t length = -1)  { String* string = new String(s, length); _stringStore.push_back(string); return string; }
    static String* createString(const String& s)  { String* string = new String(s); _stringStore.push_back(string); return string; }
    
protected:
    void setProto(Object* obj) { _proto = obj; }
    Object* proto() const { return _proto; }
    
    void setHasIterator(bool b) { _hasIterator = b; }
    void setHasGet(bool b) { _hasGet = b; }
    void setHasSet(bool b) { _hasSet = b; }
    
private:
    void _gcMark(ExecutionUnit*);

    Object* _proto = nullptr;
    bool _marked : 1;
    bool _hasIterator : 1;
    bool _hasGet : 1;
    bool _hasSet : 1;

    static std::vector<String*> _stringStore;
    static std::vector<Object*> _objectStore;
    static std::vector<Object*> _staticObjects;
};

class MaterObject : public Object {
public:
    MaterObject(bool isArray = false) : _isArray(isArray) { }
    virtual ~MaterObject();

    virtual String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark(ExecutionUnit* eu) override;
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override;
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append) override;

    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override;
    virtual const Value property(ExecutionUnit* eu, const Atom& prop) const override;
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& v, Value::SetPropertyType type) override;
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;

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
    Object* _iterator = nullptr;
    bool _isArray = false;
    bool _arrayNeedsGC;
};

class NativeFunction : public Object {
public:
    typedef CallReturnValue (*Func)(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
    NativeFunction(Func func);
    
    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const override { return typeOnly ? String("NativeFunction") : Object::toString(eu, false); }

    virtual CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor) override { return _func(eu, thisValue, nparams); }

private:
    Func _func = nullptr;
};

class NativeObject {
public:
    NativeObject() { }
    virtual ~NativeObject() { }

    virtual void gcMark(ExecutionUnit* eu) { }
};

class ObjectFactory {
public:
    ObjectFactory(Program*, Atom name);
    ~ObjectFactory();
    
    void addProperty(Atom prop, Object*);
    void addProperty(Atom prop, const Value&);
    
    Object* nativeObject() { return &_obj; }
    const Object* nativeObject() const { return &_obj; }
    
    static Object* create(Atom objectName, ExecutionUnit*, uint32_t nparams);

protected:
    MaterObject _obj;
};
    
}
