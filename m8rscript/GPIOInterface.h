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
    
    void enableHeartbeat() { setPinMode(LED, PinMode::Output); }
    void heartbeat(bool on)
    {
        if (_pinMode[LED] != PinMode::Output) {
            return;
        }
        
        // Generally the heartbeat is the inverse of the current state of the LED pin. But when turning
        // it off (which will be for a longer period of time) if the pin has changed state from when
        // we turned it on, we assume it is being used somewhere else, so we don't change it
        bool state = digitalRead(LED);
        if ((!on && (state ^ _heartbeatState)) || (on == _heartbeatState)) {
            _heartbeatState = on;
            return;
        }
        _heartbeatState = !state;
        digitalWrite(LED, _heartbeatState);
    }
    
private:
    PinMode _pinMode[PinCount];
    bool _heartbeatState = false;
};

}
