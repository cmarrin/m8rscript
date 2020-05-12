/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MacSystemInterface.h"
#include "Application.h"
#include "GPIOInterface.h"

extern "C" {

m8r::Application* application = nullptr;

void m8rInitialize(const char* fsFile)
{
    if (!application) {
        m8r::initMacFileSystem(fsFile);
        application = new m8r::Application(800);
    }
}

void m8rRunOneIteration()
{
    if (application) {
        application->runOneIteration();
    }
}

const char* m8rGetConsoleString()
{
    return "";
}

void m8rGetGPIOState(uint32_t* value, uint32_t* change)
{
    m8r::system()->gpio()->getState(*value, *change);
}

}
