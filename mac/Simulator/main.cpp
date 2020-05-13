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

static m8r::Application* application = nullptr;

static m8r::String consoleString; 

void m8rInitialize(const char* fsFile)
{
    if (!application) {
        m8r::initMacSystemInterface(fsFile, [](const char* s) {
            consoleString += s;
        });
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
    m8r::String s = std::move(consoleString);
    return s.c_str();
}

void m8rGetGPIOState(uint32_t* value, uint32_t* change)
{
    m8r::system()->gpio()->getState(*value, *change);
}

}
