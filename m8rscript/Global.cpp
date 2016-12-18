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

//static const uint32_t BASE64_STACK_ALLOC_LIMIT = 32;

Global::Global(SystemInterface* system, Program* program)
    : _system(system)
    , _serial(program)
{
    program->addObject(this, false);
    
    _startTime = 0;

    _DateAtom = program->atomizeString(ROMSTR("Date"));
    _SystemAtom = program->atomizeString(ROMSTR("System"));
    _SerialAtom = program->atomizeString(ROMSTR("Serial"));
    _GPIOAtom = program->atomizeString(ROMSTR("GPIO"));
    _Base64Atom = program->atomizeString(ROMSTR("Base64"));
    
    _nowAtom = program->atomizeString(ROMSTR("now"));
    _delayAtom = program->atomizeString(ROMSTR("delay"));
    _setPinModeAtom = program->atomizeString(ROMSTR("setPinMode"));
    _digitalWriteAtom = program->atomizeString(ROMSTR("digitalWrite"));
    _digitalReadAtom = program->atomizeString(ROMSTR("digitalRead"));
    _onInterruptAtom = program->atomizeString(ROMSTR("onInterrupt"));
    
    _PinModeAtom = program->atomizeString(ROMSTR("PinMode"));
    _TriggerAtom = program->atomizeString(ROMSTR("Trigger"));

    _OutputAtom = program->atomizeString(ROMSTR("Output"));
    _OutputOpenDrainAtom = program->atomizeString(ROMSTR("OutputOpenDrain"));
    _InputAtom = program->atomizeString(ROMSTR("Input"));
    _InputPullupAtom = program->atomizeString(ROMSTR("InputPullup"));
    _InputPulldownAtom = program->atomizeString(ROMSTR("InputPulldown"));
    _NoneAtom = program->atomizeString(ROMSTR("None"));
    _RisingEdgeAtom = program->atomizeString(ROMSTR("RisingEdge"));
    _FallingEdgeAtom = program->atomizeString(ROMSTR("FallingEdge"));
    _BothEdgesAtom = program->atomizeString(ROMSTR("BothEdges"));
    _LowAtom = program->atomizeString(ROMSTR("Low"));
    _HighAtom = program->atomizeString(ROMSTR("High"));

    _encodeAtom = program->atomizeString(ROMSTR("encode"));
    _decodeAtom = program->atomizeString(ROMSTR("decode"));
}

Global::~Global()
{
}

const Value Global::property(const Atom& name) const
{
    if (name == _SerialAtom) {
        return Value(_serial.objectId());
    }
    return Value();
}

bool Global::setProperty(ExecutionUnit*, const Atom& name, const Value& value)
{
    return false;
//    switch(static_cast<Property>(index)) {
//        case Property::Date: return true;
//        default: return false;
//    }
}

Atom Global::propertyName(uint32_t index) const
{
    Property property = static_cast<Property>(index);
    switch(property) {
        case Property::Date: return _DateAtom;
        case Property::System: return _SystemAtom;
        case Property::Serial: return _SerialAtom;
        case Property::GPIO: return _GPIOAtom;
        case Property::Base64: return _Base64Atom;
        default: return Atom();
    }
}

size_t Global::propertyCount() const
{
    return PropertyCount;
}

//CallReturnValue Global::callProperty(uint32_t index, ExecutionUnit* eu, uint32_t nparams)
//{
//    switch(static_cast<Property>(index)) {
//        case Property::Date_now: {
//            uint64_t t = _system->currentMicroseconds() - _startTime;
//            eu->stack().push(Float(static_cast<Float::value_type>(t), -6));
//            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
//        }
//        case Property::Serial_print:
//            for (int i = 1 - nparams; i <= 0; ++i) {
//                if (_system) {
//                    _system->printf(eu->stack().top(i).toStringValue(eu).c_str());
//                }
//            }
//            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//        case Property::Serial_printf:
//            for (int i = 1 - nparams; i <= 0; ++i) {
//                if (_system) {
//                    _system->printf(eu->stack().top(i).toStringValue(eu).c_str());
//                }
//            }
//            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//        case Property::Base64_encode: {
//            String inString = eu->stack().top().toStringValue(eu);
//            size_t inLength = inString.size();
//            size_t outLength = (inLength * 4 + 2) / 3 + 1;
//            if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
//                char outString[BASE64_STACK_ALLOC_LIMIT];
//                int actualLength = base64_encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()), 
//                                                 BASE64_STACK_ALLOC_LIMIT, outString);
//                StringId stringId = eu->program()->createString();
//                String& s = eu->program()->str(stringId);
//                s = String(outString, actualLength);
//                eu->stack().push(stringId);
//            } else {
//                char* outString = static_cast<char*>(malloc(outLength));
//                int actualLength = base64_encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()),
//                                                 BASE64_STACK_ALLOC_LIMIT, outString);
//                StringId stringId = eu->program()->createString();
//                String& s = eu->program()->str(stringId);
//                s = String(outString, actualLength);
//                eu->stack().push(stringId);
//                free(outString);
//            }
//            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
//        }
//        case Property::Base64_decode: {
//            String inString = eu->stack().top().toStringValue(eu);
//            size_t inLength = inString.size();
//            size_t outLength = (inLength * 3 + 3) / 4 + 1;
//            if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
//                unsigned char outString[BASE64_STACK_ALLOC_LIMIT];
//                int actualLength = base64_decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString);
//                StringId stringId = eu->program()->createString();
//                String& s = eu->program()->str(stringId);
//                s = String(reinterpret_cast<char*>(outString), actualLength);
//                eu->stack().push(stringId);
//            } else {
//                unsigned char* outString = static_cast<unsigned char*>(malloc(outLength));
//                int actualLength = base64_decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString);
//                StringId stringId = eu->program()->createString();
//                String& s = eu->program()->str(stringId);
//                s = String(reinterpret_cast<char*>(outString), actualLength);
//                eu->stack().push(stringId);
//                free(outString);
//            }
//            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
//        }
//        case Property::System_delay: {
//            uint32_t ms = eu->stack().top().toIntValue(eu);
//            return CallReturnValue(CallReturnValue::Type::MsDelay, ms);
//        }
//        case Property::GPIO_setPinMode: {
//            uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
//            GPIO::PinMode mode = (nparams >= 2) ? static_cast<GPIO::PinMode>(eu->stack().top(2 - nparams).toIntValue(eu)) : GPIO::PinMode::Input;
//            _system->gpio().setPinMode(pin, mode);
//            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//        }
//        case Property::GPIO_digitalWrite: {
//            uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
//            bool level = (nparams >= 2) ? eu->stack().top(2 - nparams).toBoolValue(eu) : false;
//            _system->gpio().digitalWrite(pin, level);
//            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//        }
//        default: return CallReturnValue(CallReturnValue::Type::Error);
//    }
//}

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
