/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"

m8r::Application _application(23);

void systemInitialized()
{
    _application.runLoop();
}

extern "C" void user_init()
{
    initializeSystem(systemInitialized);
}
