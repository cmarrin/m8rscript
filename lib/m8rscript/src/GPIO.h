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
#include "Object.h"
#include <functional>

namespace m8r {

#if M8RSCRIPT_SUPPORT == 1
class PinMode : public StaticObject {
public:
    PinMode();
};

class Trigger : public StaticObject {
public:
    Trigger();
};

class GPIO : public StaticObject {
public:
    GPIO();
    
    static PinMode _pinMode;
    static Trigger _trigger;
    
    static CallReturnValue setPinMode(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue digitalWrite(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue digitalRead(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue onInterrupt(ExecutionUnit*, Value thisValue, uint32_t nparams);
};
#endif

}
