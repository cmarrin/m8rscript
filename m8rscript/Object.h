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
#include "Value.h"

namespace m8r {

class Object {
public:
    typedef std::vector<Value::Map::Pair> Properties;

    virtual ~Object() { }

    virtual const Atom* name() const { return nullptr; }
    
    virtual bool hasCode() const { return false; }
    virtual uint8_t codeAtIndex(uint32_t index) const { return 0; }
    virtual uint32_t codeSize() const { return 0; }
    
    virtual const Properties* properties() const { return nullptr; }
    virtual int32_t propertyIndex(const Atom& s) const { return -1; }
    virtual Value* property(int32_t index) { return nullptr; }
    virtual bool setProperty(int32_t index, const Value&) { return false; }
    virtual Atom propertyName(uint32_t index) const { return Atom(); }
    virtual int32_t addProperty(const Atom&) { return -1; }
    
    virtual int32_t addLocal(const Atom& name) { return -1; }
    virtual int32_t localIndex(const Atom& name) const { return -1; }

    virtual bool setValue(const Value&) { return false; }
    virtual Value* value() { return nullptr; }
};
    
class MaterObject : public Object {
public:    
    virtual ~MaterObject() { }

    virtual const Properties* properties() const override { return &_properties; }
    virtual int32_t propertyIndex(const Atom& s) const override { return findPropertyIndex(s); }
    virtual Value* property(int32_t index) override { return &(_properties[index].value); }
    virtual bool setProperty(int32_t index, const Value& v) override
    {
        _properties[index].value = v;
        return true;
    }
    
    virtual Atom propertyName(uint32_t index) const override { return _properties[index].key; }
    virtual int32_t addProperty(const Atom& name) override
    {
        if (findPropertyIndex(name) >= 0) {
            return -1;
        }
        _properties.push_back({ name, Value() });
        return static_cast<int32_t>(_properties.size()) - 1;
    }

private:
    int32_t findPropertyIndex(const Atom& name) const
    {
        for (int32_t i = 0; i < _properties.size(); ++i) {
            if (_properties[i].key == name) {
                return i;
            }
        }
        return -1;
    }
    
    Properties _properties;
};
    
}
