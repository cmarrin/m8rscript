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

using namespace m8r;

static const uint32_t BASE64_STACK_ALLOC_LIMIT = 32;

Global::Global(SystemInterface* system, Program* program) : _system(system)
{
    _startTime = 0;

    _DateAtom = program->atomizeString("Date");
    _SystemAtom = program->atomizeString("System");
    _SerialAtom = program->atomizeString("Serial");
    _GPIOAtom = program->atomizeString("GPIO");
    _Base64Atom = program->atomizeString("Base64");
    
    _nowAtom = program->atomizeString("now");
    _delayAtom = program->atomizeString("delay");
    _pinModeAtom = program->atomizeString("pinMode");
    _digitalWriteAtom = program->atomizeString("digitalWrite");
    _digitalReadAtom = program->atomizeString("digitalRead");
    _onInterruptAtom = program->atomizeString("onInterrupt");
    
    _OutputAtom =       program->atomizeString("Output");
    _InputAtom =        program->atomizeString("Input");
    _OpenDrainAtom =    program->atomizeString("OpenDrain");
    _InterruptAtom =    program->atomizeString("Interrupt");
    _NoneAtom =         program->atomizeString("None");
    _RisingEdgeAtom =   program->atomizeString("RisingEdge");
    _FallingEdgeAtom =  program->atomizeString("FallingEdge");
    _BothEdgesAtom =    program->atomizeString("BothEdges");
    _LowAtom =          program->atomizeString("Low");
    _HighAtom =         program->atomizeString("High");

    _beginAtom = program->atomizeString("begin");
    _printAtom = program->atomizeString("print");
    _printfAtom = program->atomizeString("printf");
    _encodeAtom = program->atomizeString("encode");
    _decodeAtom = program->atomizeString("decode");
}

Global::~Global()
{
}

int32_t Global::propertyIndex(const Atom& name)
{
    if (name == _DateAtom) {
        return static_cast<int32_t>(Property::Date);
    }
    if (name == _SystemAtom) {
        return static_cast<int32_t>(Property::System);
    }
    if (name == _SerialAtom) {
        return static_cast<int32_t>(Property::Serial);
    }
    if (name == _GPIOAtom) {
        return static_cast<int32_t>(Property::GPIO);
    }
    if (name == _Base64Atom) {
        return static_cast<int32_t>(Property::Base64);
    }
    return -1;
}

Value Global::propertyRef(int32_t index)
{
    return Value(this, index, true);
}

const Value Global::property(int32_t index) const
{
    switch(static_cast<Property>(index)) {
        case Property::Date: return Value(0);
        case Property::GPIO_Output: return Value(static_cast<uint32_t>(GPIO::PinMode::Output));
        case Property::GPIO_Input: return Value(static_cast<uint32_t>(GPIO::PinMode::Input));
        case Property::GPIO_OpenDrain: return Value(static_cast<uint32_t>(GPIO::PinMode::OpenDrain));
        case Property::GPIO_Interrupt: return Value(static_cast<uint32_t>(GPIO::PinMode::Interrupt));
        case Property::GPIO_None: return Value(static_cast<uint32_t>(GPIO::Trigger::None));
        case Property::GPIO_RisingEdge: return Value(static_cast<uint32_t>(GPIO::Trigger::RisingEdge));
        case Property::GPIO_FallingEdge: return Value(static_cast<uint32_t>(GPIO::Trigger::FallingEdge));
        case Property::GPIO_BothEdges: return Value(static_cast<uint32_t>(GPIO::Trigger::BothEdges));
        case Property::GPIO_Low: return Value(static_cast<uint32_t>(GPIO::Trigger::Low));
        case Property::GPIO_High: return Value(static_cast<uint32_t>(GPIO::Trigger::High));
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
            } else if (name == _printfAtom) {
                newProperty = Property::Serial_printf;
            }
            break;
        case Property::GPIO:
            if (name == _OutputAtom) {
                newProperty = Property::GPIO_Output;
            } else if (name == _InputAtom) {
                newProperty = Property::GPIO_Input;
            } else if (name == _OpenDrainAtom) {
                newProperty = Property::GPIO_OpenDrain;
            } else if (name == _InterruptAtom) {
                newProperty = Property::GPIO_Interrupt;
            } else if (name == _NoneAtom) {
                newProperty = Property::GPIO_None;
            } else if (name == _RisingEdgeAtom) {
                newProperty = Property::GPIO_RisingEdge;
            } else if (name == _FallingEdgeAtom) {
                newProperty = Property::GPIO_FallingEdge;
            } else if (name == _BothEdgesAtom) {
                newProperty = Property::GPIO_BothEdges;
            } else if (name == _LowAtom) {
                newProperty = Property::GPIO_Low;
            } else if (name == _HighAtom) {
                newProperty = Property::GPIO_High;
            } else if (name == _pinModeAtom) {
                newProperty = Property::GPIO_pinMode;
            } else if (name == _digitalWriteAtom) {
                newProperty = Property::GPIO_digitalWrite;
            }
            break;
        case Property::Base64:
            if (name == _encodeAtom) {
                newProperty = Property::Base64_encode;
            } else if (name == _decodeAtom) {
                newProperty = Property::Base64_decode;
            }
            break;
        default:
            break;
    }

    return (newProperty == Property::None) ? Value() : Value(this, static_cast<uint16_t>(newProperty), true);
}

CallReturnValue Global::callProperty(uint32_t index, ExecutionUnit* eu, uint32_t nparams)
{
    switch(static_cast<Property>(index)) {
        case Property::Date_now: {
            uint64_t t = _system->currentMicroseconds() - _startTime;
            eu->stack().push(Float(static_cast<int32_t>(t), -6));
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
        }
        case Property::Serial_print:
            for (int i = 1 - nparams; i <= 0; ++i) {
                if (_system) {
                    _system->printf(eu->stack().top(i).toStringValue(eu->program()).c_str());
                }
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        case Property::Serial_printf:
            for (int i = 1 - nparams; i <= 0; ++i) {
                if (_system) {
                    _system->printf(eu->stack().top(i).toStringValue(eu->program()).c_str());
                }
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        case Property::Base64_encode: {
            String inString = eu->stack().top().toStringValue(eu->program());
            size_t inLength = inString.size();
            size_t outLength = (inLength * 4 + 2) / 3 + 1;
            if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
                char outString[BASE64_STACK_ALLOC_LIMIT];
                int actualLength = base64_encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()), 
                                                 BASE64_STACK_ALLOC_LIMIT, outString);
                eu->stack().push(Value(outString, actualLength));
            } else {
                char* outString = static_cast<char*>(malloc(outLength));
                int actualLength = base64_encode(inLength, reinterpret_cast<const uint8_t*>(inString.c_str()),
                                                 BASE64_STACK_ALLOC_LIMIT, outString);
                eu->stack().push(Value(outString, actualLength));
                free(outString);
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
        }
        case Property::Base64_decode: {
            String inString = eu->stack().top().toStringValue(eu->program());
            size_t inLength = inString.size();
            size_t outLength = (inLength * 3 + 3) / 4 + 1;
            if (outLength <= BASE64_STACK_ALLOC_LIMIT) {
                unsigned char outString[BASE64_STACK_ALLOC_LIMIT];
                int actualLength = base64_decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString);
                eu->stack().push(Value(reinterpret_cast<char*>(outString), actualLength));
            } else {
                unsigned char* outString = static_cast<unsigned char*>(malloc(outLength));
                int actualLength = base64_decode(inLength, inString.c_str(), BASE64_STACK_ALLOC_LIMIT, outString);
                eu->stack().push(Value(reinterpret_cast<char*>(outString), actualLength));
                free(outString);
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
        }
        case Property::System_delay: {
            uint32_t ms = eu->stack().top().toUIntValue();
            return CallReturnValue(CallReturnValue::Type::MsDelay, ms);
        }
        case Property::GPIO_pinMode: {
            uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toUIntValue() : 0;
            GPIO::PinMode mode = (nparams >= 2) ? static_cast<GPIO::PinMode>(eu->stack().top(2 - nparams).toUIntValue()) : GPIO::PinMode::Input;
            bool pullup = (nparams >= 3) ? eu->stack().top(3 - nparams).toBoolValue() : false;
            _system->gpio().pinMode(pin, mode, pullup);
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        case Property::GPIO_digitalWrite: {
            uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toUIntValue() : 0;
            bool level = (nparams >= 2) ? eu->stack().top(2 - nparams).toBoolValue() : false;
            _system->gpio().digitalWrite(pin, level);
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        default: return CallReturnValue(CallReturnValue::Type::Error);
    }
}
