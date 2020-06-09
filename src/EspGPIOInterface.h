/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "GPIOInterface.h"

#include "Esp.h"
#include <ets_sys.h>

namespace m8r {

class EspGPIOInterface : public GPIOInterface {
public:
    EspGPIOInterface();
    virtual ~EspGPIOInterface();
    
    virtual bool setPinMode(uint8_t pin, PinMode mode) override;
    virtual bool digitalRead(uint8_t pin) const override;
    virtual void digitalWrite(uint8_t pin, bool level) override;
    virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) override;    

private:
    static constexpr uint8_t InvalidName = 0xff;
    
    struct PinEntry {
        PinEntry(uint32_t name, uint8_t func) : _name(name - PERIPHS_IO_MUX), _func(func) { }
        uint8_t _name;
        uint8_t _func;
    };
    
    static PinEntry getPinEntry(uint8_t pin)
    {
        return { ROMString::readByte(ROMString(&(_pins[pin * 2]))), ROMString::readByte(ROMString(&(_pins[pin * 2 + 1]))) };
    }
    
    static const uint8_t _pins[PinCount * 2];
};

}
