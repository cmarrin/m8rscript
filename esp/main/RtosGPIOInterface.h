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

namespace m8r {

class RtosGPIOInterface : public GPIOInterface {
public:
    RtosGPIOInterface();
    virtual ~RtosGPIOInterface();
    
    virtual bool setPinMode(uint8_t pin, PinMode mode) override;
    virtual bool digitalRead(uint8_t pin) const override;
    virtual void digitalWrite(uint8_t pin, bool level) override;
    virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) override;    

private:
};

}
