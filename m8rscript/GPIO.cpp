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
    : ObjectFactory(program, ROMSTR("GPIO"))
    , _pinMode(program)
    , _trigger(program)
    , _setPinMode(setPinMode)
    , _digitalWrite(digitalWrite)
    , _digitalRead(digitalRead)
    , _onInterrupt(onInterrupt)
{
    addProperty(program, ATOM(setPinMode), &_setPinMode);
    addProperty(program, ATOM(digitalWrite), &_digitalWrite);
    addProperty(program, ATOM(digitalRead), &_digitalRead);
    addProperty(program, ATOM(onInterrupt), &_onInterrupt);
    
    addProperty(program, ATOM(PinMode), Value(_pinMode.nativeObject()));
    addProperty(program, ATOM(Trigger), Value(_trigger.nativeObject()));
}

CallReturnValue GPIO::setPinMode(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
    GPIOInterface::PinMode mode = (nparams >= 2) ? static_cast<GPIOInterface::PinMode>(eu->stack().top(2 - nparams).toIntValue(eu)) : GPIOInterface::PinMode::Input;
    eu->system()->gpio().setPinMode(pin, mode);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue GPIO::digitalWrite(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
    bool level = (nparams >= 2) ? eu->stack().top(2 - nparams).toBoolValue(eu) : false;
    eu->system()->gpio().digitalWrite(pin, level);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue GPIO::digitalRead(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Type::Error);
}

CallReturnValue GPIO::onInterrupt(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Type::Error);
}

PinMode::PinMode(Program* program)
    : ObjectFactory(program, ROMSTR("PinMode"))
{
    addProperty(program, ATOM(Output), Value(static_cast<int32_t>(GPIOInterface::PinMode::Output)));
    addProperty(program, ATOM(OutputOpenDrain), Value(static_cast<int32_t>(GPIOInterface::PinMode::OutputOpenDrain)));
    addProperty(program, ATOM(Input), Value(static_cast<int32_t>(GPIOInterface::PinMode::Input)));
    addProperty(program, ATOM(InputPullup), Value(static_cast<int32_t>(GPIOInterface::PinMode::InputPullup)));
    addProperty(program, ATOM(InputPulldown), Value(static_cast<int32_t>(GPIOInterface::PinMode::InputPulldown)));
}

Trigger::Trigger(Program* program)
    : ObjectFactory(program, ROMSTR("Trigger"))
{
    addProperty(program, ATOM(None), Value(static_cast<int32_t>(GPIOInterface::Trigger::None)));
    addProperty(program, ATOM(RisingEdge), Value(static_cast<int32_t>(GPIOInterface::Trigger::RisingEdge)));
    addProperty(program, ATOM(FallingEdge), Value(static_cast<int32_t>(GPIOInterface::Trigger::FallingEdge)));
    addProperty(program, ATOM(BothEdges), Value(static_cast<int32_t>(GPIOInterface::Trigger::BothEdges)));
    addProperty(program, ATOM(Low), Value(static_cast<int32_t>(GPIOInterface::Trigger::Low)));
    addProperty(program, ATOM(High), Value(static_cast<int32_t>(GPIOInterface::Trigger::High)));
}
