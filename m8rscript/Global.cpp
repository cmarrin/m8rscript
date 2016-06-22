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

#include "Global.h"

#include "Program.h"
#ifdef __APPLE__
#include <ctime>
#else
extern "C" {
#include<user_interface.h>
}
#endif

using namespace m8r;

Map<Atom, Global::Property> Global::_properties;

Global::Global(void (*printer)(const char*)) : _printer(printer)
{
    _startTime = 0;
    _startTime = currentTime();

    if (_properties.empty()) {
        _properties.emplace(Program::atomizeString("Date"), Property::Date);
        _properties.emplace(Program::atomizeString("now"), Property::Date_now);
        _properties.emplace(Program::atomizeString("print"), Property::print);
    }
}

int32_t Global::propertyIndex(const Atom& name, bool canExist)
{
    Property prop;
    return _properties.find(name, prop) ? static_cast<int32_t>(prop) : -1;
}

Value Global::propertyRef(int32_t index)
{
    switch(static_cast<Property>(index)) {
        case Property::Date: return Value(this, static_cast<uint32_t>(Property::Date), true);
        case Property::print: return Value(this, static_cast<uint32_t>(Property::print), true);
        default: return Value();
    }
}

const Value Global::property(int32_t index) const
{
    switch(static_cast<Property>(index)) {
        case Property::Date: return Value(0);
        default: return Value();
    }
}

bool Global::setProperty(int32_t index, const Value& value)
{
    switch(static_cast<Property>(index)) {
        case Property::Date: return true;
        default: return false;
    }
}

Atom Global::propertyName(uint32_t index) const
{
    // Find it the hard way
    for (const auto& entry : _properties) {
        if (static_cast<int32_t>(entry.value) == index) {
            return entry.key;
        }
    }
    return Atom::emptyAtom();
}

size_t Global::propertyCount() const
{
    return _properties.size();
}

Value Global::appendPropertyRef(uint32_t index, const Atom& name)
{
    if (index != static_cast<uint32_t>(Property::Date) || name.rawAtom() != Program::atomizeString("now").rawAtom()) {
        return Value();
    }
    return Value(this, static_cast<uint16_t>(Property::Date_now), true);
}

int32_t Global::callProperty(uint32_t index, Stack<Value>& stack, uint32_t nparams)
{
    switch(static_cast<Property>(index)) {
        case Property::Date_now:
            stack.push(currentTime());
            return 1;
        case Property::print:
            for (int i = 1 - nparams; i <= 0; ++i) {
                _printer(stack.top(i).toStringValue().c_str());
            }
        default: return -1;
    }
}

uint32_t Global::currentTime() const
{
#ifdef __APPLE__
    return static_cast<uint32_t>(static_cast<uint64_t>(std::clock() * 1000000 / CLOCKS_PER_SEC) - _startTime);
#else
    return system_get_time() - _startTime;
#endif
}
