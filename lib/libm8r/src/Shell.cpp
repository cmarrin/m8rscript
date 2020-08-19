/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Shell.h"

using namespace m8r;

Shell::Shell()
{
}

CallReturnValue Shell::execute()
{
    return CallReturnValue(CallReturnValue::Type::Finished);
}
