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
    Array(Program*);

    virtual const char* typeName() const override { return "Array"; }

    virtual void gcMark(ExecutionUnit* eu) override
    {
        for (auto entry : _array) {
            entry.gcMark(eu);
        }
    }
    
    virtual const Value element(int32_t index) const override { return (index >= 0 && index < _array.size()) ? _array[index] : Value(); }
    virtual bool setElement(ExecutionUnit*, int32_t index, const Value& value) override
    {
        if (index < 0 || index >= _array.size()) {
            return false;
        }
        _array[index] = value;
        return true;
    }
    virtual const Value element(ExecutionUnit* eu, const Value& elt) const override { return element(elt.toIntValue(eu)); }
    virtual bool setElement(ExecutionUnit* eu, const Value& elt, const Value& value) override { return setElement(eu, elt.toIntValue(eu), value); }
    virtual bool appendElement(ExecutionUnit*, const Value& value) override { _array.push_back(value); return true; }
    virtual size_t elementCount() const override { return _array.size(); }
    virtual void setElementCount(size_t size) override { _array.resize(size); }

    // Array has built-in properties. Handle those here
    virtual const Value property(const Atom& prop) const override;
    virtual bool setProperty(ExecutionUnit*, const Atom& prop, const Value&) override;
    virtual Atom propertyName(uint32_t index) const override;
    virtual size_t propertyCount() const override;

protected:
    virtual bool serialize(Stream*, Error&, Program*) const override
    {
        // FIXME: Implement
        return false;
    }

    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override
    {
        // FIXME: Implement
        return false;
    }

private:
    static constexpr size_t PropertyCount = 1; // length

    enum class Property : uint8_t { Length };

    Atom _lengthAtom;
    
    std::vector<Value> _array;
};
    
}
