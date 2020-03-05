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
    std::thread thread([this]{
        while (true) {
            {
                std::unique_lock<std::mutex> lock(_eventMutex);
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
    _eventThread.swap(thread);
}

MacTaskManager::~MacTaskManager()
{
    _terminating = true;
    readyToExecuteNextTask();
    _eventThread.join();
}

void MacTaskManager::readyToExecuteNextTask()
{
    std::unique_lock<std::mutex> lock(_eventMutex);
    _eventCondition.notify_all();
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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
