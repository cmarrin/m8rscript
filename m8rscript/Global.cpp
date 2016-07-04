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
#include "SystemInterface.h"
#include "ExecutionUnit.h"

using namespace m8r;

Atom Global::_nowAtom;
Atom Global::_delayAtom;
Atom Global::_pinModeAtom;
Atom Global::_digitalWriteAtom;
Atom Global::_OUTPUTAtom;
Atom Global::_INPUTAtom;
Atom Global::_LOWAtom;
Atom Global::_HIGHAtom;
Atom Global::_FLOATAtom;
Atom Global::_PULLUPAtom;
Atom Global::_INTAtom;
Atom Global::_OPENDRAINAtom;
Atom Global::_beginAtom;
Atom Global::_printAtom;

Map<Atom, Global::Property> Global::_properties;

Global::Global(SystemInterface* system) : _system(system)
{
    _startTime = 0;

    if (_properties.empty()) {
        _nowAtom = Program::atomizeString("now");
        _delayAtom = Program::atomizeString("delay");
        _pinModeAtom = Program::atomizeString("pinMode");
        _digitalWriteAtom = Program::atomizeString("digitalWrite");
        _OUTPUTAtom = Program::atomizeString("OUTPUT");
        _INPUTAtom = Program::atomizeString("INPUT");
        _LOWAtom = Program::atomizeString("LOW");
        _HIGHAtom = Program::atomizeString("HIGH");
        _FLOATAtom = Program::atomizeString("FLOAT");
        _PULLUPAtom = Program::atomizeString("PULLUP");
        _INTAtom = Program::atomizeString("INT");
        _OPENDRAINAtom = Program::atomizeString("OPENDRAIN");
        _beginAtom = Program::atomizeString("begin");
        _printAtom = Program::atomizeString("print");

        _properties.emplace(Program::atomizeString("Date"), Property::Date);
        _properties.emplace(Program::atomizeString("System"), Property::System);
        _properties.emplace(Program::atomizeString("Serial"), Property::Serial);
        _properties.emplace(Program::atomizeString("GPIO"), Property::GPIO);
    }
}

Global::~Global()
{
}

int32_t Global::propertyIndex(const Atom& name, bool canExist)
{
    Property prop;
    return _properties.find(name, prop) ? static_cast<int32_t>(prop) : -1;
}

Value Global::propertyRef(int32_t index)
{
    return Value(this, index, true);
}

const Value Global::property(int32_t index) const
{
    switch(static_cast<Property>(index)) {
        case Property::Date: return Value(0);
        case Property::GPIO_OUTPUT: return Value(PLATFORM_GPIO_OUTPUT);
        case Property::GPIO_INPUT: return Value(PLATFORM_GPIO_INPUT);
        case Property::GPIO_HIGH: return Value(PLATFORM_GPIO_HIGH);
        case Property::GPIO_LOW: return Value(PLATFORM_GPIO_LOW);
        case Property::GPIO_FLOAT: return Value(PLATFORM_GPIO_FLOAT);
        case Property::GPIO_PULLUP: return Value(PLATFORM_GPIO_PULLUP);
        case Property::GPIO_INT: return Value(PLATFORM_GPIO_INT);
        case Property::GPIO_OPENDRAIN: return Value(PLATFORM_GPIO_OPENDRAIN);
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
        if (static_cast<uint32_t>(entry.value) == index) {
            return entry.key;
        }
    }
    return Atom();
}

size_t Global::propertyCount() const
{
    return _properties.size();
}

Value Global::appendPropertyRef(uint32_t index, const Atom& name)
{
    Property newProperty = Property::None;
    switch(static_cast<Property>(index)) {
        case Property::Date:
            if (name == _nowAtom) {
                newProperty = Property::Date_now;
            }
            break;
        case Property::System:
            if (name == _delayAtom) {
                newProperty = Property::System_delay;
            }
            break;
        case Property::Serial:
            if (name == _beginAtom) {
                newProperty = Property::Serial_begin;
            } else if (name == _printAtom) {
                newProperty = Property::Serial_print;
            }
            break;
        case Property::GPIO:
            if (name == _OUTPUTAtom) {
                newProperty = Property::GPIO_OUTPUT;
            } else if (name == _LOWAtom) {
                newProperty = Property::GPIO_LOW;
            } else if (name == _HIGHAtom) {
                newProperty = Property::GPIO_HIGH;
            } else if (name == _pinModeAtom) {
                newProperty = Property::GPIO_pinMode;
            } else if (name == _digitalWriteAtom) {
                newProperty = Property::GPIO_digitalWrite;
            }
            break;
        default:
            break;
    }

    return (newProperty == Property::None) ? Value() : Value(this, static_cast<uint16_t>(newProperty), true);
}

int32_t Global::callProperty(uint32_t index, Program* program, ExecutionUnit* eu, uint32_t nparams)
{
    switch(static_cast<Property>(index)) {
        case Property::Date_now: {
            uint64_t t = currentTime();
            eu->stack().push(Float(static_cast<int32_t>(t), -6));
            return 1;
        }
        case Property::Serial_print:
            for (int i = 1 - nparams; i <= 0; ++i) {
                if (_system) {
                    _system->print(eu->stack().top(i).toStringValue().c_str());
                }
            }
            return 0;
        default: return -1;
    }
}
