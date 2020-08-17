/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2020, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Object.h"

namespace m8r {

class TimerProto : public StaticObject {
public:
    TimerProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue start(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue stop(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
