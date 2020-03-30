/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Program.h"

#include "ExecutionUnit.h"

using namespace m8r;

Program::Program()
{
    // Set a dummy 'consoleListener' property so it can be overwritten
    setProperty(Atom(SA::consoleListener), Value::NullValue());
}

Program::~Program()
{
}
