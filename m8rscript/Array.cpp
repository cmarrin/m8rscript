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

#include "Array.h"

#include "Program.h"

using namespace m8r;

Map<Atom, Array::Property> Array::_properties;

Array::Array()
{
    if (_properties.empty()) {
        _properties.emplace(Program::atomizeString("length"), Property::Length);
    }
}

int32_t Array::propertyIndex(const Atom& name, bool canExist)
{
    Property prop;
    return _properties.find(name, prop) ? static_cast<int32_t>(prop) : -1;
}

Value Array::propertyRef(int32_t index)
{
    switch(static_cast<Property>(index)) {
        case Property::Length: return Value(this, static_cast<int32_t>(Property::Length));
        default: return Value();
    }
}

const Value Array::property(int32_t index) const
{
    switch(static_cast<Property>(index)) {
        case Property::Length: return Value(static_cast<int32_t>(_array.size()));
        default: return Value();
    }
}

bool Array::setProperty(int32_t index, const Value& value)
{
    switch(static_cast<Property>(index)) {
        case Property::Length: _array.resize(value.toUIntValue()); return true;
        default: return false;
    }
}

Atom Array::propertyName(uint32_t index) const
{
    switch(static_cast<Property>(index)) {
        case Property::Length:
            // Find it the hard way
            for (const auto& entry : _properties) {
                if (static_cast<int32_t>(entry.value) == index) {
                    return entry.key;
                }
            }
            return Atom::emptyAtom();
        default: return Atom::emptyAtom();
    }
}

size_t Array::propertyCount() const
{
    return _properties.size();
}
