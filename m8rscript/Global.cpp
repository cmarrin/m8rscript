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

Global::Global(SystemInterface* system, Program* program) : _system(system)
{
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

    _beginAtom = program->atomizeString(ROMSTR("begin"));
    _printAtom = program->atomizeString(ROMSTR("print"));
    _printfAtom = program->atomizeString(ROMSTR("printf"));
    _encodeAtom = program->atomizeString(ROMSTR("encode"));
    _decodeAtom = program->atomizeString(ROMSTR("decode"));
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
    return Value(objectId(), index, true);
}

const Value Global::property(int32_t index) const
{
    switch(static_cast<Property>(index)) {
        case Property::Date: return Value(0);
        case Property::GPIO_PinMode_Output: return Value(static_cast<uint32_t>(GPIO::PinMode::Output));
        case Property::GPIO_PinMode_OutputOpenDrain: return Value(static_cast<uint32_t>(GPIO::PinMode::OutputOpenDrain));
        case Property::GPIO_PinMode_Input: return Value(static_cast<uint32_t>(GPIO::PinMode::Input));
        case Property::GPIO_PinMode_InputPullup: return Value(static_cast<uint32_t>(GPIO::PinMode::InputPullup));
        case Property::GPIO_PinMode_InputPulldown: return Value(static_cast<uint32_t>(GPIO::PinMode::InputPulldown));
        case Property::GPIO_Trigger_None: return Value(static_cast<uint32_t>(GPIO::Trigger::None));
        case Property::GPIO_Trigger_RisingEdge: return Value(static_cast<uint32_t>(GPIO::Trigger::RisingEdge));
        case Property::GPIO_Trigger_FallingEdge: return Value(static_cast<uint32_t>(GPIO::Trigger::FallingEdge));
        case Property::GPIO_Trigger_BothEdges: return Value(static_cast<uint32_t>(GPIO::Trigger::BothEdges));
        case Property::GPIO_Trigger_Low: return Value(static_cast<uint32_t>(GPIO::Trigger::Low));
        case Property::GPIO_Trigger_High: return Value(static_cast<uint32_t>(GPIO::Trigger::High));
        default: return Value();
    }
}

bool Global::setProperty(ExecutionUnit*, int32_t index, const Value& value)
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
            if (name == _PinModeAtom) {
                newProperty = Property::GPIO_PinMode;
            } else if (name == _TriggerAtom) {
                newProperty = Property::GPIO_Trigger;
            } else if (name == _setPinModeAtom) {
                newProperty = Property::GPIO_setPinMode;
            } else if (name == _digitalWriteAtom) {
                newProperty = Property::GPIO_digitalWrite;
            } else if (name == _digitalReadAtom) {
                newProperty = Property::GPIO_digitalWrite;
            } else if (name == _onInterruptAtom) {
                newProperty = Property::GPIO_digitalWrite;
            }
            break;
        case Property::GPIO_PinMode:
            if (name == _OutputAtom) {
                newProperty = Property::GPIO_PinMode_Output;
            } else if (name == _OutputOpenDrainAtom) {
                newProperty = Property::GPIO_PinMode_OutputOpenDrain;
            } else if (name == _InputAtom) {
                newProperty = Property::GPIO_PinMode_Input;
            } else if (name == _InputPullupAtom) {
                newProperty = Property::GPIO_PinMode_InputPullup;
            } else if (name == _InputPulldownAtom) {
                newProperty = Property::GPIO_PinMode_InputPulldown;
            }
            break;
        case Property::GPIO_Trigger:
            if (name == _NoneAtom) {
                newProperty = Property::GPIO_Trigger_None;
            } else if (name == _RisingEdgeAtom) {
                newProperty = Property::GPIO_Trigger_RisingEdge;
            } else if (name == _FallingEdgeAtom) {
                newProperty = Property::GPIO_Trigger_FallingEdge;
            } else if (name == _BothEdgesAtom) {
                newProperty = Property::GPIO_Trigger_BothEdges;
            } else if (name == _LowAtom) {
                newProperty = Property::GPIO_Trigger_Low;
            } else if (name == _HighAtom) {
                newProperty = Property::GPIO_Trigger_High;
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

    return (newProperty == Property::None) ? Value() : Value(objectId(), static_cast<uint16_t>(newProperty), true);
}

CallReturnValue Global::callProperty(uint32_t index, ExecutionUnit* eu, uint32_t nparams)
{
    switch(static_cast<Property>(index)) {
        case Property::Date_now: {
            uint64_t t = _system->currentMicroseconds() - _startTime;
            eu->stack().push(Float(static_cast<Float::value_type>(t), -6));
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
        }
        case Property::Serial_print:
            for (int i = 1 - nparams; i <= 0; ++i) {
                if (_system) {
                    _system->printf(eu->stack().top(i).toStringValue(eu).c_str());
                }
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        case Property::Serial_printf:
            for (int i = 1 - nparams; i <= 0; ++i) {
                if (_system) {
                    _system->printf(eu->stack().top(i).toStringValue(eu).c_str());
                }
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        case Property::Base64_encode: {
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
        case Property::Base64_decode: {
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
        case Property::System_delay: {
            uint32_t ms = eu->stack().top().toIntValue(eu);
            return CallReturnValue(CallReturnValue::Type::MsDelay, ms);
        }
        case Property::GPIO_setPinMode: {
            uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
            GPIO::PinMode mode = (nparams >= 2) ? static_cast<GPIO::PinMode>(eu->stack().top(2 - nparams).toIntValue(eu)) : GPIO::PinMode::Input;
            _system->gpio().setPinMode(pin, mode);
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        case Property::GPIO_digitalWrite: {
            uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
            bool level = (nparams >= 2) ? eu->stack().top(2 - nparams).toBoolValue(eu) : false;
            _system->gpio().digitalWrite(pin, level);
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        default: return CallReturnValue(CallReturnValue::Type::Error);
    }
}
