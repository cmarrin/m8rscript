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

namespace m8r {

class Error;
class ExecutionUnit;
class Object;
class Program;
class Stream;

class Object {    
public:
    virtual ~Object() { }

    virtual String toString(ExecutionUnit* eu, bool typeOnly = false) const { return typeOnly ? String() : toString(eu, true) + " { }"; }
    
    virtual void gcMark(ExecutionUnit* eu) { _gcMark(eu); }
    
    virtual const Value property(ExecutionUnit*, const Atom&) const { return Value(); }
    
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& value, Value::SetPropertyType) { return false; }
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const { return property(eu, elt.toIdValue(eu)); }
    
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
    {
        return setProperty(eu, elt.toIdValue(eu), value, append ? Value::SetPropertyType::AlwaysAdd : Value::SetPropertyType::NeverAdd);
    }

    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) { return CallReturnValue(CallReturnValue::Type::Error); }
    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) { return CallReturnValue(CallReturnValue::Type::Error); }
    
    static constexpr int32_t IteratorCount = -1;
    static constexpr int32_t IteratorNext = -2;
    virtual Value iteratedValue(ExecutionUnit*, int32_t index) const { return Value(); }
    virtual bool setIteratedValue(ExecutionUnit*, int32_t index, const Value&, Value::SetPropertyType) { return false; }

    void setCollectable(bool b) { _collectable = b; }
    bool isCollectable() const { return _collectable; }
    
    void setMarked(bool b) { _marked = b; }
    bool isMarked() const { return _marked; }
    
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

    static void gc(ExecutionUnit*);
    static void gcMark(const Value& value);
    static void gcMark(Object* obj) { if (obj) obj->setMarked(true); }
    
    static String* createString() { String* string = new String(); _stringStore.push_back(string); return string; }
    static String* createString(const char* s, int32_t length = -1)  { String* string = new String(s, length); _stringStore.push_back(string); return string; }
    static String* createString(const String& s)  { String* string = new String(s); _stringStore.push_back(string); return string; }
    
protected:
    void setProto(Object* obj) { _proto = obj; }
    Object* proto() const { return _proto; }
    
private:
    void _gcMark(ExecutionUnit*);

    Object* _proto = nullptr;
    bool _collectable = false;
    bool _marked = true;

    static std::vector<String*> _stringStore;
    static std::vector<Object*> _objectStore;
    static std::vector<Object*> _staticObjects;
};

class MaterObject : public Object {
public:
    MaterObject() { }
    virtual ~MaterObject();

    virtual String toString(ExecutionUnit*, bool typeOnly = false) const override;

    virtual void gcMark(ExecutionUnit* eu) override;
    
    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override;
    virtual const Value property(ExecutionUnit* eu, const Atom& prop) const override;
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& v, Value::SetPropertyType type) override;
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams, bool ctor) override;

    virtual Value iteratedValue(ExecutionUnit* eu, int32_t index) const override
    {
        if (index == Object::IteratorCount) {
            return Value(static_cast<int32_t>(_properties.size()));
        }
        return Value((_properties.size() > index) ? (_properties.begin() + index)->key : Atom());
    }
    
private:
    void removeNoncollectableObjects();

    Value::Map _properties;

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
    ObjectFactory(Program*, const char* name = nullptr);
    ~ObjectFactory();
    
    void addProperty(Program*, Atom prop, Object*);
    void addProperty(Program*, Atom prop, const Value&);
    
    Object* nativeObject() { return &_obj; }
    const Object* nativeObject() const { return &_obj; }
    
    static Object* create(Atom objectName, ExecutionUnit*, uint32_t nparams);

protected:
    MaterObject _obj;
};
    
}
