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
    
    enum class PinMode { Invalid, Output, OutputOpenDrain, Input, InputPullup, InputPulldown };
    enum class Trigger { None, RisingEdge, FallingEdge, BothEdges, Low, High };
    
    GPIOInterface()
    {
        for (auto& p : _pinMode) {
            p = PinMode::Invalid;
        }
    }
    virtual ~GPIOInterface() { }
    
    virtual bool setPinMode(uint8_t pin, PinMode mode = PinMode::Input)
    {
        if (pin >= PinCount) {
            return false;
        }
        _pinMode[pin] = mode;

        _pinIO = (_pinIO & ~(1 << pin)) | ((mode == PinMode::Output) ? (1 << pin) : 0);
        _pinChanged |= (1 << pin);

        return true;
    }
    
    virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) { }

    virtual bool digitalRead(uint8_t pin) const
    {
        return _pinState & (1 << pin);
    }
    
    virtual void digitalWrite(uint8_t pin, bool level)
    {
        if (pin > 16) {
            return;
        }
        if (level) {
            _pinState |= (1 << pin);
        } else {
            _pinState &= ~(1 << pin);
        }
        _pinChanged |= (1 << pin);
    }
    
    uint8_t builtinLED() const { return LED; }
    
    void getState(uint32_t& value, uint32_t& change)
    {
        value = _pinState;
        change = _pinChanged;
        _pinChanged = 0;
    }
    
    PinMode pinMode(uint8_t pin)
    {
        if (pin >= PinCount) {
            return PinMode::Invalid;
        }
        return _pinMode[pin];
    }
    
private:
    PinMode _pinMode[PinCount];

    // 0 = input, 1 = output
    uint32_t _pinIO = 0;
    uint32_t _pinState = 0;
    uint32_t _pinChanged = 0;
};

}
