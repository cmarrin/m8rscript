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

class TCPProto : public StaticObject {
public:
    TCPProto();

    static m8r::CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue send(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue disconnect(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
