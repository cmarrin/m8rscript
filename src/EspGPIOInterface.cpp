/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

/* Some code: lifted from:
 * Copyright (c) 2015, eadf (https://github.com/eadf)
 * All rights reserved.
 */

#include "EspGPIOInterface.h"

#include <gpio.h>

using namespace m8r;

EspGPIOInterface::EspGPIOInterface()
{
}

EspGPIOInterface::~EspGPIOInterface()
{
}

bool EspGPIOInterface::setPinMode(uint8_t pin, PinMode mode)
{
    if (!GPIOInterface::setPinMode(pin, mode)) {
        return false;
    }

    uint8_t m = INPUT;
    switch (mode) {
        case PinMode::Output: m = OUTPUT; break;
        case PinMode::OutputOpenDrain: m = OUTPUT_OPEN_DRAIN; break;
        case PinMode::Input: m = INPUT; break;
        case PinMode::InputPullup: m = INPUT_PULLUP; break;

        // FIXME: Not supported on Arduino
        case PinMode::InputPulldown: m = INPUT_PULLUP; break;
    }
    
    pinMode(pin, m);
    return true;
}

bool EspGPIOInterface::digitalRead(uint8_t pin) const
{
    return ::digitalRead(pin) != LOW;
}

void EspGPIOInterface::digitalWrite(uint8_t pin, bool level)
{
    ::digitalWrite(pin, level ? HIGH : LOW);
}

void EspGPIOInterface::onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)>)
{
}
