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
#include "base64.h"
#include <string>

using namespace m8r;

static const uint32_t BASE64_STACK_ALLOC_LIMIT = 32;

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

const Value Global::property(const Atom& name) const
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

Atom Global::propertyName(uint32_t index) const
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

size_t Global::propertyCount() const
{
    return PropertyCount;
}

Serial::Serial(Program* program)
    : _begin(begin)
    , _print(print)
    , _printf(printf)
{
    _beginAtom = program->atomizeString(ROMSTR("begin"));
    _printAtom = program->atomizeString(ROMSTR("print"));
    _printfAtom = program->atomizeString(ROMSTR("printf"));
    
    program->addObject(this, false);
    
    program->addObject(&_begin, false);
    program->addObject(&_print, false);
    program->addObject(&_printf, false);
}

CallReturnValue Serial::begin(ExecutionUnit*, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Type::Error);
}

CallReturnValue Serial::print(ExecutionUnit* eu, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        SystemInterface::shared()->printf(eu->stack().top(i).toStringValue(eu).c_str());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Serial::printf(ExecutionUnit*, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Type::Error);
}

const Value Serial::property(const Atom& prop) const
{
    if (prop == _beginAtom) {
        return Value(_begin.objectId());
    }
    if (prop == _printAtom) {
        return Value(_print.objectId());
    }
    if (prop == _printfAtom) {
        return Value(_printf.objectId());
    }
    return Value();
}

System::System(Program* program)
    : _delay(delay)
{
    _delayAtom = program->atomizeString(ROMSTR("delay"));    
    program->addObject(this, false);
    program->addObject(&_delay, false);
}

CallReturnValue System::delay(ExecutionUnit* eu, uint32_t nparams)
{
    uint32_t ms = eu->stack().top().toIntValue(eu);
    return CallReturnValue(CallReturnValue::Type::MsDelay, ms);
}

const Value System::property(const Atom& prop) const
{
    if (prop == _delayAtom) {
        return Value(_delay.objectId());
    }
    return Value();
}

Date::Date(Program* program)
    : _now(now)
{
    _nowAtom = program->atomizeString(ROMSTR("now"));    
    program->addObject(this, false);
    program->addObject(&_now, false);
}

CallReturnValue Date::now(ExecutionUnit* eu, uint32_t nparams)
{
    uint64_t t = SystemInterface::shared()->currentMicroseconds();
    eu->stack().push(Float(static_cast<Float::value_type>(t), -6));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

const Value Date::property(const Atom& prop) const
{
    if (prop == _nowAtom) {
        return Value(_now.objectId());
    }
    return Value();
}

Base64::Base64(Program* program)
    : _encode(encode)
    , _decode(decode)
{
    _encodeAtom = program->atomizeString(ROMSTR("encode"));    
    _decodeAtom = program->atomizeString(ROMSTR("decode"));    
    program->addObject(this, false);
    program->addObject(&_encode, false);
    program->addObject(&_decode, false);
}

CallReturnValue Base64::encode(ExecutionUnit* eu, uint32_t nparams)
{
    String inString = eu->stack().top().toStringValue(eu);
    size_t inLength = inString.size();
    size_t outLength = (inLength * 4 + 2) / 3 + 1;
    if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
        char outString[BASE64_STACK_ALLOC_LIMIT];
        int actualLength = base64_encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()), 
                                         BASE64_STACK_ALLOC_LIMIT, outString);
        StringId stringId = eu->program()->createString();
        String& s = eu->program()->str(stringId);
        s = String(outString, actualLength);
        eu->stack().push(stringId);
    } else {
        char* outString = static_cast<char*>(malloc(outLength));
        int actualLength = base64_encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()),
                                         BASE64_STACK_ALLOC_LIMIT, outString);
        StringId stringId = eu->program()->createString();
        String& s = eu->program()->str(stringId);
        s = String(outString, actualLength);
        eu->stack().push(stringId);
        free(outString);
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Base64::decode(ExecutionUnit* eu, uint32_t nparams)
{
    String inString = eu->stack().top().toStringValue(eu);
    size_t inLength = inString.size();
    size_t outLength = (inLength * 3 + 3) / 4 + 1;
    if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
        unsigned char outString[BASE64_STACK_ALLOC_LIMIT];
        int actualLength = base64_decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString);
        StringId stringId = eu->program()->createString();
        String& s = eu->program()->str(stringId);
        s = String(reinterpret_cast<char*>(outString), actualLength);
        eu->stack().push(stringId);
    } else {
        unsigned char* outString = static_cast<unsigned char*>(malloc(outLength));
        int actualLength = base64_decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString);
        StringId stringId = eu->program()->createString();
        String& s = eu->program()->str(stringId);
        s = String(reinterpret_cast<char*>(outString), actualLength);
        eu->stack().push(stringId);
        free(outString);
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

const Value Base64::property(const Atom& prop) const
{
    if (prop == _encodeAtom) {
        return Value(_encode.objectId());
    }
    if (prop == _decodeAtom) {
        return Value(_decode.objectId());
    }
    return Value();
}
