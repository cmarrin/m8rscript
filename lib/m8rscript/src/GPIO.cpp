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

PinMode GPIO::_pinMode;
Trigger GPIO::_trigger;

static StaticObject::StaticFunctionProperty RODATA2_ATTR _funcPropsGPIO[] =
{
    { SA::setPinMode, GPIO::setPinMode },
    { SA::digitalWrite, GPIO::digitalWrite },
    { SA::digitalRead, GPIO::digitalRead },
    { SA::onInterrupt, GPIO::onInterrupt },
};

static StaticObject::StaticObjectProperty  _objPropsGPIO[] =
{
    { SA::PinMode, &GPIO::_pinMode },
    { SA::Trigger, &GPIO::_trigger },
};

GPIO::GPIO()
{
    setProperties(_funcPropsGPIO, sizeof(_funcPropsGPIO) / sizeof(StaticFunctionProperty));
    setProperties(_objPropsGPIO, sizeof(_objPropsGPIO) / sizeof(StaticObjectProperty));
}

CallReturnValue GPIO::setPinMode(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (!system()->gpio()) {
        return CallReturnValue(CallReturnValue::Error::Unimplemented);
    }
    
    uint8_t pin = (nparams >= 1) ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
    GPIOInterface::PinMode mode = (nparams >= 2) ? static_cast<GPIOInterface::PinMode>(eu->stack().top(2 - nparams).toIntValue(eu)) : GPIOInterface::PinMode::Input;
    system()->gpio()->setPinMode(pin, mode);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue GPIO::digitalWrite(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (!system()->gpio()) {
        return CallReturnValue(CallReturnValue::Error::Unimplemented);
    }
    
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

static StaticObject::StaticProperty _propsPinMode[] =
{
    { SA::Output, Value(static_cast<int32_t>(GPIOInterface::PinMode::Output)) },
    { SA::OutputOpenDrain, Value(static_cast<int32_t>(GPIOInterface::PinMode::OutputOpenDrain)) },
    { SA::Input, Value(static_cast<int32_t>(GPIOInterface::PinMode::Input)) },
    { SA::InputPullup, Value(static_cast<int32_t>(GPIOInterface::PinMode::InputPullup)) },
    { SA::InputPulldown, Value(static_cast<int32_t>(GPIOInterface::PinMode::InputPulldown)) },
};

PinMode::PinMode()
{
    setProperties(_propsPinMode, sizeof(_propsPinMode) / sizeof(StaticProperty));
}

static StaticObject::StaticProperty _propsTrigger[] =
{
    { SA::None, Value(static_cast<int32_t>(GPIOInterface::Trigger::None)) },
    { SA::RisingEdge, Value(static_cast<int32_t>(GPIOInterface::Trigger::RisingEdge)) },
    { SA::FallingEdge, Value(static_cast<int32_t>(GPIOInterface::Trigger::FallingEdge)) },
    { SA::BothEdges, Value(static_cast<int32_t>(GPIOInterface::Trigger::BothEdges)) },
    { SA::Low, Value(static_cast<int32_t>(GPIOInterface::Trigger::Low)) },
    { SA::High, Value(static_cast<int32_t>(GPIOInterface::Trigger::High)) },
};

Trigger::Trigger()
{
    setProperties(_propsTrigger, sizeof(_propsTrigger) / sizeof(StaticProperty));
}
