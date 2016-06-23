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

#include "Object.h"

namespace m8r {

class Array : public Object {
public:
    Array();

    virtual const char* typeName() const override { return "Array"; }

    virtual Value elementRef(int32_t index) override { return Value(this, index, false); }
    virtual const Value element(uint32_t index) const override { return (index < _array.size()) ? _array[index] : Value(); }
    virtual bool setElement(uint32_t index, const Value& value) override
    {
        if (index >= _array.size()) {
            return false;
        }
        _array[index] = value;
        return true;
    }
    virtual bool appendElement(const Value& value) override { _array.push_back(value); return true; }
    virtual size_t elementCount() const override { return _array.size(); }
    virtual void setElementCount(size_t size) override { _array.resize(size); }

    // Array has built-in properties. Handle those here
    virtual int32_t propertyIndex(const Atom& s, bool canExist) override;
    virtual Value propertyRef(int32_t index) override;
    virtual const Value property(int32_t index) const override;
    virtual bool setProperty(int32_t index, const Value&) override;
    virtual Atom propertyName(uint32_t index) const override;
    virtual size_t propertyCount() const override;

private:
    enum class Property { Length };
    static Map<Atom, Property> _properties;
    Vector<Value> _array;
};
    
}
