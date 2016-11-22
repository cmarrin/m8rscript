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

#pragma once

#include "GPIO.h"

#include "Esp.h"
#include <ets_sys.h>

namespace m8r {

class EspGPIO : public GPIO {
public:
    EspGPIO();
    virtual ~EspGPIO();
    
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
        uint16_t v = readRomShort(reinterpret_cast<const uint16_t*>(&(_pins[pin * 2])));
        return *(reinterpret_cast<PinEntry*>(&v));
    }
    
    static const uint8_t _pins[PinCount * 2];
};

}
