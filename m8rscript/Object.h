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
class Program;
class Stream;

class Object {
public:
    virtual ~Object() { }
    
    static void* operator new (size_t size)
    {
        void *p = malloc(size);
        assert(p);
        return p;
    }
    static void operator delete (void *p) { free(p); }
    
    virtual const char* typeName() const = 0;
    
    virtual bool isFunction() const { return false; }

    virtual String toString(ExecutionUnit* eu) const { return String(typeName()) + " { }"; }
    
    virtual void gcMark(ExecutionUnit* eu) { if (_proto) { _gcMark(eu); } }
    
    virtual const Value property(ExecutionUnit*, const Atom&) const { return Value(); }
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& value, bool add) { return false; }
    
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const { return property(eu, elt.toIdValue(eu)); }
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append) { return setProperty(eu, elt.toIdValue(eu), value, append); }
    
    virtual CallReturnValue construct(ExecutionUnit*, uint32_t nparams) { return CallReturnValue(CallReturnValue::Type::Error); }
    virtual CallReturnValue call(ExecutionUnit*, Value thisValue, uint32_t nparams) { return CallReturnValue(CallReturnValue::Type::Error); }
    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) { return CallReturnValue(CallReturnValue::Type::Error); }
    
    static constexpr int32_t IteratorCount = -1;
    static constexpr int32_t IteratorNext = -2;
    virtual Value iteratedValue(ExecutionUnit*, int32_t index) const { return Value(); }
    virtual bool setIteratedValue(ExecutionUnit*, int32_t index, const Value&) { return false; }

    bool serializeObject(Stream*, Error&, Program*) const;
    bool deserializeObject(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&);

    virtual bool serialize(Stream*, Error&, Program*) const { return true; }
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) { return true; }

    void setObjectId(ObjectId id) { _objectId = id; }
    ObjectId objectId() const { return _objectId; }
    
    void setCollectable(bool b) { _collectable = b; }
    bool collectable() const { return _collectable; }
    
protected:
    void setProto(ObjectId id) { _proto = id; }
    ObjectId proto() const { return _proto; }
    
    bool serializeBuffer(Stream*, Error&, ObjectDataType, const uint8_t* buffer, size_t size) const;
    
    bool serializeWrite(Stream*, Error&, ObjectDataType) const;
    bool serializeWrite(Stream*, Error&, uint8_t) const;
    bool serializeWrite(Stream*, Error&, uint16_t) const;

    bool deserializeBufferSize(Stream*, Error&, ObjectDataType, uint16_t& size) const;
    bool deserializeBuffer(Stream* stream, Error&, uint8_t* buffer, uint16_t size) const;

    bool deserializeRead(Stream*, Error&, ObjectDataType&) const;
    bool deserializeRead(Stream*, Error&, uint8_t&) const;
    bool deserializeRead(Stream*, Error&, uint16_t&) const;

private:
    void _gcMark(ExecutionUnit*);

    ObjectId _proto;
    ObjectId _objectId;
    bool _collectable = false;
};

// This is an object with a property map, which is read only from m8rscript
class PropertyObject : public Object {
    friend class ObjectFactory;
    
public:    
    virtual ~PropertyObject() { }

    virtual const char* typeName() const override { return ""; }
    virtual String toString(ExecutionUnit* eu) const override;

    virtual void gcMark(ExecutionUnit* eu) override
    {
        Object::gcMark(eu);
        for (auto entry : _properties) {
            entry.value.gcMark(eu);
        }
    }
    
    virtual CallReturnValue callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams) override;

    virtual const Value property(ExecutionUnit* eu, const Atom& prop) const override
    {
        auto it = _properties.find(prop);
        return (it != _properties.end()) ? it->value : Value();
    }

    bool setProperty(const Atom& prop, const Value& v, bool add);
    
    virtual CallReturnValue construct(ExecutionUnit*, uint32_t nparams) override;

    virtual Value iteratedValue(ExecutionUnit* eu, int32_t index) const override
    {
        if (index == Object::IteratorCount) {
            return Value(static_cast<int32_t>(_properties.size()));
        }
        return Value((_properties.size() > index) ? (_properties.begin() + index)->key : Atom());
    }

protected:
    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;
    
    void removeNoncollectableObjects();

    Value::Map _properties;
};

class MaterObject : public PropertyObject {
public:    
    virtual ~MaterObject() { }

    virtual const char* typeName() const override { return "Object"; }
    virtual String toString(ExecutionUnit*) const override;

    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& v, bool add) override
    {
        return PropertyObject::setProperty(prop, v, add);
    }
};

class NativeFunction : public Object {
public:
    typedef CallReturnValue (*Func)(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
    NativeFunction(Func func) : _func(func) { }
    
    virtual const char* typeName() const override { return "NativeFunction"; }

    virtual CallReturnValue call(ExecutionUnit* eu, Value thisValue, uint32_t nparams) override { return _func(eu, thisValue, nparams); }

private:
    Func _func = nullptr;
};

class Closure {
public:
    Closure(const Value& func) : _func(func) { }
    
    void addArg(const Value& value) { _args.push_back(value); }
    void clearArgs() { _args.clear(); }
    
private:
    Value _func;
    std::vector<Value> _args;
};

class ObjectFactory {
public:
    ObjectFactory(Program*, const char* name = nullptr);
    ~ObjectFactory() { _obj.removeNoncollectableObjects(); }
    
    void addProperty(Program*, Atom prop, Object*);
    void addProperty(Program*, Atom prop, const Value&);
    
    Object* nativeObject() { return &_obj; }
    const Object* nativeObject() const { return &_obj; }
    ObjectId objectId() const { return _obj.objectId(); }

protected:
    PropertyObject _obj;
};
    
}
