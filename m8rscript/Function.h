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
#include "Containers.h"
#include "Atom.h"

namespace m8r {

class Function : public Object {
public:
    Function() { _name.set(Atom::NoAtom); }

    virtual ~Function()
    {
        for (auto obj : _objects) {
            delete obj;
        }
    }

    virtual const Atom* name() const override { return &_name; }
    virtual bool hasCode() const override { return true; }
    virtual uint8_t codeAtIndex(uint32_t index) const override { return _code[index]; }
    virtual uint32_t numObjects() const override { return static_cast<uint32_t>(_objects.size()); }
    virtual Object* objectAtIndex(uint32_t i) override { return _objects[i]; }
    virtual String stringFromCode(uint32_t index, uint32_t len) const override
    {
        return String(reinterpret_cast<const char*>(&(_code[index])), len);
    }

    void setName(const Atom& atom) { _name = atom; }
    void addParam(const Atom& atom) { _params.push_back(atom); }
    
    void addCode(uint8_t c) { _code.push_back(c); }
    void setCodeAtIndex(uint32_t index, uint8_t c) { _code[index] = c; }
    uint32_t codeSize() const { return static_cast<uint32_t>(_code.size()); }
    
    static uint8_t byteFromInt(uint32_t value, uint32_t index)
    {
        assert(index < 4);
        return static_cast<uint8_t>(value >> (8 * index));
    }
        
    void addCodeInt(uint32_t value, uint32_t size)
    {
        for (int i = size - 1; i >= 0; --i) {
            addCode(byteFromInt(value, i));
        }
    }
    
    void addObject(Object* obj) { _objects.push_back(obj); }
    
    const Vector<Object*>& objects() const { return _objects; }

private:
    Vector<Atom> _params;
    Vector<uint8_t> _code;
    Vector<Object*> _objects;
	Atom _name;
};
    
}
