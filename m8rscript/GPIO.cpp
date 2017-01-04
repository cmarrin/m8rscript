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

#include "GPIO.h"

#include "ExecutionUnit.h"
#include "GPIOInterface.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

GPIO::GPIO(Program* program)
    : _pinMode(program)
    , _trigger(program)
{
    _setPinModeAtom = program->atomizeString(ROMSTR("setPinMode"));    
    _digitalWriteAtom = program->atomizeString(ROMSTR("digitalWrite"));    
    _digitalReadAtom = program->atomizeString(ROMSTR("digitalRead"));    
    _onInterruptAtom = program->atomizeString(ROMSTR("onInterrupt"));
    _PinModeAtom = program->atomizeString(ROMSTR("PinMode"));
    _TriggerAtom = program->atomizeString(ROMSTR("Trigger"));
    
    program->addObject(this, false);
}

const Value GPIO::property(ExecutionUnit*, const Atom& prop) const
{
    if (prop == _PinModeAtom) {
        return Value(_pinMode.objectId());
    }
    if (prop == _TriggerAtom) {
        return Value(_trigger.objectId());
    }
    return Value();
}

CallReturnValue GPIO::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    if (prop == _setPinModeAtom) {
        uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
        GPIOInterface::PinMode mode = (nparams >= 2) ? static_cast<GPIOInterface::PinMode>(eu->stack().top(2 - nparams).toIntValue(eu)) : GPIOInterface::PinMode::Input;
        SystemInterface::shared()->gpio().setPinMode(pin, mode);
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    if (prop == _digitalWriteAtom) {
        uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
        bool level = (nparams >= 2) ? eu->stack().top(2 - nparams).toBoolValue(eu) : false;
        SystemInterface::shared()->gpio().digitalWrite(pin, level);
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    if (prop == _digitalReadAtom) {
        // FIXME: Implement
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    if (prop == _onInterruptAtom) {
        // FIXME: Implement
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    return CallReturnValue(CallReturnValue::Type::Error);
}

PinMode::PinMode(Program* program)
{
    _OutputAtom = program->atomizeString(ROMSTR("Output"));
    _OutputOpenDrainAtom = program->atomizeString(ROMSTR("OutputOpenDrain"));
    _InputAtom = program->atomizeString(ROMSTR("Input"));
    _InputPullupAtom = program->atomizeString(ROMSTR("InputPullup"));
    _InputPulldownAtom = program->atomizeString(ROMSTR("InputPulldown"));
    
    program->addObject(this, false);
}

const Value PinMode::property(ExecutionUnit*, const Atom& prop) const
{
    if (prop == _OutputAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::PinMode::Output));
    }
    if (prop == _OutputOpenDrainAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::PinMode::OutputOpenDrain));
    }
    if (prop == _InputAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::PinMode::Input));
    }
    if (prop == _InputPullupAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::PinMode::InputPullup));
    }
    if (prop == _InputPulldownAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::PinMode::InputPulldown));
    }
    return Value();
}

Trigger::Trigger(Program* program)
{
    _NoneAtom = program->atomizeString(ROMSTR("None"));
    _RisingEdgeAtom = program->atomizeString(ROMSTR("RisingEdge"));
    _FallingEdgeAtom = program->atomizeString(ROMSTR("FallingEdge"));
    _BothEdgesAtom = program->atomizeString(ROMSTR("BothEdges"));
    _LowAtom = program->atomizeString(ROMSTR("Low"));
    _HighAtom = program->atomizeString(ROMSTR("High"));
    
    program->addObject(this, false);
}

const Value Trigger::property(ExecutionUnit*, const Atom& prop) const
{
    if (prop == _NoneAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::Trigger::None));
    }
    if (prop == _RisingEdgeAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::Trigger::RisingEdge));
    }
    if (prop == _FallingEdgeAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::Trigger::FallingEdge));
    }
    if (prop == _BothEdgesAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::Trigger::BothEdges));
    }
    if (prop == _LowAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::Trigger::Low));
    }
    if (prop == _HighAtom) {
        return Value(static_cast<uint32_t>(GPIOInterface::Trigger::High));
    }
    return Value();
}
