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

typedef std::vector<uint8_t> Code;

class Error;
class ExecutionUnit;
class Program;
class Stream;

class Object {
public:
    typedef std::vector<Value::Map::Pair> Properties;

    virtual ~Object() { }
    
    static void* operator new (size_t size)
    {
        void *p = malloc(size);
        assert(p);
        return p;
    }
    static void operator delete (void *p) { free(p); }
    
    virtual const char* typeName() const = 0;
    
    virtual const Code* code() const { return nullptr; }
    
    virtual int32_t propertyIndex(const Atom& s, bool canExist = false) { return -1; }
    virtual Value propertyRef(int32_t index) { return Value(); }
    virtual const Value property(int32_t index) const { return Value(); }
    virtual bool setProperty(int32_t index, const Value&) { return false; }
    virtual Atom propertyName(uint32_t index) const { return Atom(); }
    virtual int32_t addProperty(const Atom&) { return -1; }
    virtual size_t propertyCount() const { return 0; }
    virtual Value appendPropertyRef(uint32_t index, const Atom&) { return Value(); }
    virtual CallReturnValue callProperty(uint32_t index, ExecutionUnit*, uint32_t nparams) { return CallReturnValue(CallReturnValue::Type::Error); }
    
    virtual Value elementRef(int32_t index) { return Value(); }
    virtual const Value element(uint32_t index) const { return Value(); }
    virtual bool setElement(uint32_t index, const Value&) { return false; }
    virtual bool appendElement(const Value&) { return false; }
    virtual size_t elementCount() const { return 0; }
    virtual void setElementCount(size_t) { }
    
    virtual int32_t addLocal(const Atom& name) { return -1; }
    virtual int32_t localIndex(const Atom& name) const { return -1; }
    virtual Atom localName(int32_t index) const { return Atom(); }
    virtual size_t localSize() const { return 0; }

    virtual bool setValue(const Value&) { return false; }
    virtual Value* value() { return nullptr; }
    virtual CallReturnValue call(ExecutionUnit*, uint32_t nparams) { return CallReturnValue(CallReturnValue::Type::Error); }

    bool serializeObject(Stream*, Error&) const;
    bool deserializeObject(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&);

    virtual bool serialize(Stream*, Error&) const = 0;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) = 0;

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
};
    
class MaterObject : public Object {
public:    
    virtual ~MaterObject() { }

    virtual const char* typeName() const override { return "Object"; }

    virtual int32_t propertyIndex(const Atom& name, bool canExist = false) override
    {
        int32_t index = findPropertyIndex(name);
        if (index < 0) {
            index = static_cast<int32_t>(_properties.size());
            _properties.push_back({ name, Value() });
        }
        return index;
    }
    virtual Value propertyRef(int32_t index) override { return Value(this, index, true); }
    virtual const Value property(int32_t index) const override { return _properties[index].value; }
    virtual bool setProperty(int32_t index, const Value& v) override
    {
        _properties[index].value = v;
        return true;
    }
    
    virtual Atom propertyName(uint32_t index) const override { return _properties[index].key; }
    virtual int32_t addProperty(const Atom& name) override
    {
        int32_t index = findPropertyIndex(name);
        if (index >= 0) {
            return -1;
        }
        _properties.push_back({ name, Value() });
        return static_cast<int32_t>(_properties.size()) - 1;
    }

    virtual size_t propertyCount() const override { return _properties.size(); }

protected:
    virtual bool serialize(Stream*, Error&) const override;
    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override;

private:
    int32_t findPropertyIndex(const Atom& name) const
    {
        for (size_t i = 0; i < _properties.size(); ++i) {
            if (_properties[i].key == name) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }
    
    Properties _properties;
};
    
}
