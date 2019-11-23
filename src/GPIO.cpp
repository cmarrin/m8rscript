/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "GPIO.h"

#include "ExecutionUnit.h"
#include "GPIOInterface.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

GPIO::GPIO(ObjectFactory* parent)
    : ObjectFactory(SA::GPIO, parent)
{
    addProperty(SA::setPinMode, setPinMode);
    addProperty(SA::digitalWrite, digitalWrite);
    addProperty(SA::digitalRead, digitalRead);
    addProperty(SA::onInterrupt, onInterrupt);

    addProperty(Atom(SA::PinMode), Value(_pinMode.nativeObject()));
    addProperty(Atom(SA::Trigger), Value(_trigger.nativeObject()));
}

CallReturnValue GPIO::setPinMode(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
    GPIOInterface::PinMode mode = (nparams >= 2) ? static_cast<GPIOInterface::PinMode>(eu->stack().top(2 - nparams).toIntValue(eu)) : GPIOInterface::PinMode::Input;
    system()->gpio()->setPinMode(pin, mode);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue GPIO::digitalWrite(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
    bool level = (nparams >= 2) ? eu->stack().top(2 - nparams).toBoolValue(eu) : false;
    system()->gpio()->digitalWrite(pin, level);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue GPIO::digitalRead(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Error::Unimplemented);
}

CallReturnValue GPIO::onInterrupt(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Error::Unimplemented);
}

PinMode::PinMode()
    : ObjectFactory(SA::PinMode)
{
    addProperty(Atom(SA::Output), Value(static_cast<int32_t>(GPIOInterface::PinMode::Output)));
    addProperty(Atom(SA::OutputOpenDrain), Value(static_cast<int32_t>(GPIOInterface::PinMode::OutputOpenDrain)));
    addProperty(Atom(SA::Input), Value(static_cast<int32_t>(GPIOInterface::PinMode::Input)));
    addProperty(Atom(SA::InputPullup), Value(static_cast<int32_t>(GPIOInterface::PinMode::InputPullup)));
    addProperty(Atom(SA::InputPulldown), Value(static_cast<int32_t>(GPIOInterface::PinMode::InputPulldown)));
}

Trigger::Trigger()
    : ObjectFactory(SA::Trigger)
{
    addProperty(Atom(SA::None), Value(static_cast<int32_t>(GPIOInterface::Trigger::None)));
    addProperty(Atom(SA::RisingEdge), Value(static_cast<int32_t>(GPIOInterface::Trigger::RisingEdge)));
    addProperty(Atom(SA::FallingEdge), Value(static_cast<int32_t>(GPIOInterface::Trigger::FallingEdge)));
    addProperty(Atom(SA::BothEdges), Value(static_cast<int32_t>(GPIOInterface::Trigger::BothEdges)));
    addProperty(Atom(SA::Low), Value(static_cast<int32_t>(GPIOInterface::Trigger::Low)));
    addProperty(Atom(SA::High), Value(static_cast<int32_t>(GPIOInterface::Trigger::High)));
}
