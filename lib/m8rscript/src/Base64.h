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
#ifndef SCRIPT_SUPPORT
static_assert(0, "SCRIPT_SUPPORT not defined");
#endif
#if SCRIPT_SUPPORT == 1

#include "Object.h"

namespace m8r {

#if SCRIPT_SUPPORT == 1
class Base64 : public StaticObject {
public:
    Base64();

    static int encode(uint16_t in_len, const unsigned char *in, uint16_t out_len, char *out);
    static int decode(uint16_t in_len, const char *in, uint16_t out_len, unsigned char *out);

    static CallReturnValue encodeFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue decodeFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
};
#endif

}

#endif
