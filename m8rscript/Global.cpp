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
#include <string>

using namespace m8r;

Global::Global(Program* program)
    : _serial(program)
    , _system(program)
    , _date(program)
    , _base64(program)
    , _gpio(program)
{
    program->addObject(this, false);
    
    _DateAtom = program->atomizeString(ROMSTR("Date"));
    _SystemAtom = program->atomizeString(ROMSTR("System"));
    _SerialAtom = program->atomizeString(ROMSTR("Serial"));
    _GPIOAtom = program->atomizeString(ROMSTR("GPIO"));
    _Base64Atom = program->atomizeString(ROMSTR("Base64"));
}

Global::~Global()
{
}

const Value Global::property(ExecutionUnit*, const Atom& name) const
{
    if (name == _SerialAtom) {
        return Value(_serial.objectId());
    }
    if (name == _SystemAtom) {
        return Value(_system.objectId());
    }
    if (name == _DateAtom) {
        return Value(_date.objectId());
    }
    if (name == _Base64Atom) {
        return Value(_base64.objectId());
    }
    if (name == _GPIOAtom) {
        return Value(_gpio.objectId());
    }
    return Value();
}

Atom Global::propertyName(ExecutionUnit*, uint32_t index) const
{
    switch(index) {
        case 0: return _SerialAtom;
        case 1: return _SystemAtom;
        case 2: return _DateAtom;
        case 3: return _Base64Atom;
        case 4: return _GPIOAtom;
        default: return Atom();
    }
}

size_t Global::propertyCount(ExecutionUnit*) const
{
    return PropertyCount;
}
