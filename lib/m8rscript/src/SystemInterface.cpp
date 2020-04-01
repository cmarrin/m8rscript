/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "SystemInterface.h"

#include "Application.h"

#include "TaskManager.h"

using namespace m8r;

SystemInterface* m8r::system()
{
    return Application::system();
}

void SystemInterface::runOneIteration()
{
    taskManager()->runOneIteration();
}
