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

    virtual String toString(ExecutionUnit*) const { return String(typeName()) + "{ }"; }
    
    virtual void gcMark(ExecutionUnit*) { }
    
    virtual const Value property(const Atom&) const { return Value(); }
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& value) { return false; }
    virtual Atom propertyName(uint32_t index) const { return Atom(); }
    virtual Value* addProperty(const Atom&) { return nullptr; }
    virtual size_t propertyCount() const { return 0; }
    
    virtual const Value element(int32_t index) const { return Value(); }
    virtual bool setElement(ExecutionUnit*, int32_t index, const Value&) { return false; }
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const { return property(elt.toIdValue(eu)); }
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value) { return setProperty(eu, elt.toIntValue(eu), value); }
    virtual bool appendElement(ExecutionUnit*, const Value&) { return false; }
    virtual size_t elementCount() const { return 0; }
    virtual void setElementCount(size_t) { }
    
    virtual bool setValue(ExecutionUnit*, const Value&) { return false; }
    virtual Value* value() { return nullptr; }
    virtual CallReturnValue call(ExecutionUnit*, uint32_t nparams) { return CallReturnValue(CallReturnValue::Type::Error); }

    bool serializeObject(Stream*, Error&, Program*) const;
    bool deserializeObject(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&);

    virtual bool serialize(Stream*, Error&, Program*) const { return true; }
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) { return true; }

    void setObjectId(ObjectId id) { _objectId = id; }
    ObjectId objectId() const { return _objectId; }
    
    void setCollectable(bool b) { _collectable = b; }
    bool collectable() const { return _collectable; }
    
protected:    
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
    ObjectId _objectId;
    bool _collectable = false;
};

class PseudoObject : public Object {
public:
    
private:
    Object* _master;
    uint32_t _index;
};
    
class MaterObject : public Object {
public:    
    virtual ~MaterObject() { }

    virtual const char* typeName() const override { return "Object"; }

    virtual String toString(ExecutionUnit*) const override;

    virtual void gcMark(ExecutionUnit* eu) override
    {
        for (auto entry : _properties) {
            entry.value.gcMark(eu);
        }
    }
    
    virtual const Value property(const Atom& prop) const override
    {
        auto it = _properties.find(prop);
        return (it != _properties.end()) ? it->value : Value();
    }
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value& v) override
    {
        auto it = _properties.find(prop);
        if (it == _properties.end()) {
            return false;
        }
        it->value = v;
        return true;
    }
    
    virtual Atom propertyName(uint32_t index) const override { return (_properties.begin() + index)->key; }
    virtual Value* addProperty(const Atom& prop) override
    {
        auto it = _properties.find(prop);
        if (it != _properties.end()) {
            return nullptr;
        }
        auto ret = _properties.emplace(prop, Value());
        assert(ret.second);
        return &(ret.first->value);
    }

    virtual size_t propertyCount() const override { return _properties.size(); }

protected:
    virtual bool serialize(Stream*, Error&, Program*) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;

private:
    Value::Map _properties;
};

class NativeFunction : public Object {
public:
    typedef CallReturnValue (*Func)(ExecutionUnit*, uint32_t nparams);
    
    NativeFunction(Func func) : _func(func) { }
    
    virtual const char* typeName() const override { return "NativeFunction"; }

    virtual CallReturnValue call(ExecutionUnit* eu, uint32_t nparams) override { return _func(eu, nparams); }

private:
    Func _func = nullptr;
};
    
}
