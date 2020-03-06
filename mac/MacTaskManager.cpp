/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MacTaskManager.h"

#include "Defines.h"
#include "SystemInterface.h"
#include <cassert>

using namespace m8r;

MacTaskManager::MacTaskManager()
{
    Thread thread([this]{
        while (true) {
            {
                Lock lock(_eventMutex);
                if (!ready()) {
                    _eventCondition.wait(lock);
                    if (_terminating) {
                        break;
                    }
                }
            }
            
            executeNextTask();
        }
    });
    thread.detach();
    _eventThread.swap(thread);
    _eventThread.detach();
}

MacTaskManager::~MacTaskManager()
{
    _terminating = true;
    readyToExecuteNextTask();
    _eventThread.join();
}

void MacTaskManager::readyToExecuteNextTask()
{
    Lock lock(_eventMutex);
    _eventCondition.notify(true);
}

void MacTaskManager::runLoop()
{
    // The mac main thread has to keep running
    while(1) {
        // Check for key input
        char* buffer = nullptr;
        size_t size = 0;
        getline(&buffer, &size, stdin);
        system()->receivedLine(buffer);
        this_thread::sleep_for(50_ms);
    }
}
