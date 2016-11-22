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

/* Some code: lifted from:
 * Copyright (c) 2015, eadf (https://github.com/eadf)
 * All rights reserved.
 */

#include "EspGPIO.h"

extern "C" {
#include <gpio.h>
#include <osapi.h>
}

using namespace m8r;

#define PinEntry(name, func) (static_cast<uint8_t>(name - PERIPHS_IO_MUX)), func

const uint8_t ICACHE_RODATA_ATTR ICACHE_STORE_ATTR EspGPIO::_pins[PinCount * 2] = {
    PinEntry(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0),
    PinEntry(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1),
    PinEntry(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2),
    PinEntry(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3),
    PinEntry(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4),
    PinEntry(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5),
    PinEntry(InvalidName, 0),
    PinEntry(InvalidName, 0),
    PinEntry(InvalidName, 0),
    PinEntry(PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9),
    PinEntry(PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10),
    PinEntry(InvalidName, 0),
    PinEntry(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12),
    PinEntry(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13),
    PinEntry(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14),
    PinEntry(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15),
    PinEntry(InvalidName, 0)
};

EspGPIO::EspGPIO()
{
}

EspGPIO::~EspGPIO()
{
}

bool EspGPIO::setPinMode(uint8_t pin, PinMode mode)
{
debugf("******** setPinMode enter:pin=%d, mode=%d\n", pin, static_cast<uint8_t>(mode));
    if (!GPIO::setPinMode(pin, mode)) {
        return false;
    }
    
debugf("******** setPinMode 1\n");
    if (pin == 16) {
        WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
            (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbcUL) | 0x1UL); // mux configuration for XPD_DCDC to output rtc_gpio0
        WRITE_PERI_REG(RTC_GPIO_CONF,
            (READ_PERI_REG(RTC_GPIO_CONF) & 0xfffffffeUL) | 0x0UL); //mux configuration for out enable
            
        if (mode == PinMode::Output) {
            WRITE_PERI_REG(RTC_GPIO_ENABLE,
                (READ_PERI_REG(RTC_GPIO_ENABLE) & 0xfffffffeUL) | 0x1UL); //out enable
        } else if (mode == PinMode::Input) {
            WRITE_PERI_REG(RTC_GPIO_ENABLE,
                READ_PERI_REG(RTC_GPIO_ENABLE) & 0xfffffffeUL);  //out disable
        } else if (mode == PinMode::InputPulldown) {
            // FIXME: Implement
        } else {
            return false;
        }
        return true;
    }
    
debugf("******** setPinMode 2\n");
    PinEntry pinEntry = getPinEntry(pin);
debugf("******** setPinMode 3\n");
    
    if (pinEntry._name == InvalidName) {
        return false;
    }

debugf("******** setPinMode 4\n");
    PIN_FUNC_SELECT(PERIPHS_IO_MUX + pinEntry._name, pinEntry._func);
debugf("******** setPinMode 5\n");

    if (mode == PinMode::InputPullup) {
        PIN_PULLUP_EN(PERIPHS_IO_MUX + pinEntry._name);
    } else {
        PIN_PULLUP_DIS(PERIPHS_IO_MUX + pinEntry._name);
    }

debugf("******** setPinMode 6\n");
    // FIXME: This will only enable output for PinMode::Output. We need to handle all the other cases
    if (mode != PinMode::Output) {
        GPIO_DIS_OUTPUT(GPIO_ID_PIN(pin));
    } else {
        gpio_output_set(0, 0, BIT(GPIO_ID_PIN(pin)), 0);
    }

debugf("******** setPinMode 7\n");
    return true;
}

bool EspGPIO::digitalRead(uint8_t pin) const
{
    return false;
}

void EspGPIO::digitalWrite(uint8_t pin, bool level)
{
}

void EspGPIO::onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)>)
{
}
