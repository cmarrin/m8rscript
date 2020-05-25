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

#include "RtosGPIOInterface.h"

#include <driver/gpio.h>

using namespace m8r;

RtosGPIOInterface::RtosGPIOInterface()
{
}

RtosGPIOInterface::~RtosGPIOInterface()
{
}

bool RtosGPIOInterface::setPinMode(uint8_t pin, PinMode mode)
{
    if (!GPIOInterface::setPinMode(pin, mode)) {
        return false;
    }
    
    gpio_config_t config;
    config.pin_bit_mask = 1 << pin;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    switch (mode) {
        case PinMode::Invalid:
            config.mode = GPIO_MODE_DISABLE;
            break;
        case PinMode::Output:
            config.mode = GPIO_MODE_OUTPUT;
            break;
        case PinMode::OutputOpenDrain: 
            config.mode = GPIO_MODE_OUTPUT_OD;
            break;
        case PinMode::Input:
            config.mode = GPIO_MODE_INPUT;
            break;
        case PinMode::InputPullup:
            config.mode = GPIO_MODE_INPUT;
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            break;
        case PinMode::InputPulldown:
            config.mode = GPIO_MODE_INPUT;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
    }
    
    gpio_config(&config);
    return true;
}

bool RtosGPIOInterface::digitalRead(uint8_t pin) const
{
    return gpio_get_level(static_cast<gpio_num_t>(pin)) != 0;
}

void RtosGPIOInterface::digitalWrite(uint8_t pin, bool level)
{
    gpio_set_level(static_cast<gpio_num_t>(pin), level ? 1 : 0);
}

void RtosGPIOInterface::onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)>)
{
}
