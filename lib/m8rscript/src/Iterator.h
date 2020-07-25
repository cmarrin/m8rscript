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
#if M8RSCRIPT_SUPPORT == 1

#include "Object.h"

namespace m8r {

#if M8RSCRIPT_SUPPORT == 1
class Iterator : public StaticObject {
public:
    Iterator();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue done(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue next(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue getValue(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue setValue(ExecutionUnit*, Value thisValue, uint32_t nparams);
};
#endif

}

#endif
