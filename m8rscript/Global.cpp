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
#include "slre.h"
#include <string>

using namespace m8r;

Global::Global(Program* program)
    : _arguments(program)
    , _base64(program)
    , _gpio(program)
    , _iterator(program)
    , _currentTime(currentTime)
    , _delay(delay)
    , _print(print)
    , _printf(printf)
    , _println(println)
{
    program->addObject(this, false);
    
    program->addObject(&_currentTime, false);
    program->addObject(&_delay, false);
    program->addObject(&_print, false);
    program->addObject(&_printf, false);
    program->addObject(&_println, false);
    
    _ArgumentsAtom = program->atomizeString(ROMSTR("arguments"));
    _Base64Atom = program->atomizeString(ROMSTR("Base64"));
    _GPIOAtom = program->atomizeString(ROMSTR("GPIO"));
    _IteratorAtom = program->atomizeString(ROMSTR("Iterator"));

    _currentTimeAtom = program->atomizeString(ROMSTR("currentTime"));    
    _delayAtom = program->atomizeString(ROMSTR("delay"));    
    _printAtom = program->atomizeString(ROMSTR("print"));
    _printfAtom = program->atomizeString(ROMSTR("printf"));
    _printlnAtom = program->atomizeString(ROMSTR("println"));
}

Global::~Global()
{
}

const Value Global::property(ExecutionUnit*, const Atom& name) const
{
    if (name == _ArgumentsAtom) {
        return Value(_arguments.objectId());
    }
    if (name == _Base64Atom) {
        return Value(_base64.objectId());
    }
    if (name == _GPIOAtom) {
        return Value(_gpio.objectId());
    }
    if (name == _IteratorAtom) {
        return Value(_iterator.objectId());
    }
    if (name == _currentTimeAtom) {
        return Value(_currentTime.objectId());
    }
    if (name == _delayAtom) {
        return Value(_delay.objectId());
    }
    if (name == _printAtom) {
        return Value(_print.objectId());
    }
    if (name == _printfAtom) {
        return Value(_printf.objectId());
    }
    if (name == _printlnAtom) {
        return Value(_println.objectId());
    }
    return Value();
}

Atom Global::propertyName(ExecutionUnit*, uint32_t index) const
{
    switch(index) {
        case 0: return _ArgumentsAtom;
        case 1: return _Base64Atom;
        case 2: return _GPIOAtom;
        case 3: return _IteratorAtom;
        case 4: return _currentTimeAtom;
        case 5: return _delayAtom;
        case 6: return _printAtom;
        case 7: return _printfAtom;
        case 8: return _printlnAtom;
        default: return Atom();
    }
}

uint32_t Global::propertyCount(ExecutionUnit*) const
{
    return 9;
}

CallReturnValue Global::currentTime(ExecutionUnit* eu, uint32_t nparams)
{
    uint64_t t = SystemInterface::shared()->currentMicroseconds();
    eu->stack().push(Float(static_cast<Float::value_type>(t), -6));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Global::delay(ExecutionUnit* eu, uint32_t nparams)
{
    uint32_t ms = eu->stack().top().toIntValue(eu);
    return CallReturnValue(CallReturnValue::Type::MsDelay, ms);
}

CallReturnValue Global::print(ExecutionUnit* eu, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        SystemInterface::shared()->printf(eu->stack().top(i).toStringValue(eu).c_str());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Global::printf(ExecutionUnit*, uint32_t nparams)
{
    slre_match(nullptr, nullptr, 0, nullptr, 0, 0);
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Type::Error);
}

CallReturnValue Global::println(ExecutionUnit* eu, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        SystemInterface::shared()->printf(eu->stack().top(i).toStringValue(eu).c_str());
    }
    SystemInterface::shared()->printf("\n");

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
