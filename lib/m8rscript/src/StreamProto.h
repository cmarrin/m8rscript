/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Object.h"

namespace m8rscript {

class StreamProto : public StaticObject {
public:
    StreamProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue eof(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue read(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue write(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
