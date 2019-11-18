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
            Time now = Time::now();
            {
                std::unique_lock<std::mutex> lock(_eventMutex);
                if (empty()) {
                    _eventCondition.wait(lock);
                    if (_terminating) {
                        break;
                    }
                }
            }
            
            Duration durationToNextEvent = nextTimeToFire() - now;
            if (durationToNextEvent <= Duration()) {
                executeNextTask();
            } else {
                std::cv_status status;
                {
                    std::unique_lock<std::mutex> lock(_eventMutex);
                    status = _eventCondition.wait_for(lock, std::chrono::milliseconds(durationToNextEvent.ms()));
                }
                if (_terminating) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    executeNextTask();
                }
            }
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
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
