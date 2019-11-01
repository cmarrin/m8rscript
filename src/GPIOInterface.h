/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Defines.h"
#include <functional>

namespace m8r {

class GPIOInterface {
public:
    static constexpr uint8_t LED = 2;
    static constexpr uint8_t PinCount = 17;
    
    enum class PinMode { Output, OutputOpenDrain, Input, InputPullup, InputPulldown };
    enum class Trigger { None, RisingEdge, FallingEdge, BothEdges, Low, High };
    
    GPIOInterface() { }
    virtual ~GPIOInterface() { }
    
    virtual bool setPinMode(uint8_t pin, PinMode mode = PinMode::Input)
    {
        if (pin >= PinCount) {
            return false;
        }
        _pinMode[pin] = mode;
        return true;
    }
    
    virtual bool digitalRead(uint8_t pin) const = 0;
    virtual void digitalWrite(uint8_t pin, bool level) = 0;
    virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) = 0;
    
    uint8_t builtinLED() const { return LED; }
    
private:
    PinMode _pinMode[PinCount];
};

}
