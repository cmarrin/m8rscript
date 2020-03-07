/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "TaskManager.h"

#include "SystemInterface.h"
#include "Task.h"
#include "Thread.h"
#include <cassert>

using namespace m8r;

static Duration MaxTaskDelay = Duration(6000000, Duration::Units::ms);
static Duration MinTaskDelay = Duration(1, Duration::Units::ms);
static Duration MainLoopSleepDelay = 50_ms;
static constexpr uint32_t MainThreadSize = 4096;
static constexpr uint32_t DelayThreadSize = 1024;

TaskManager::TaskManager()
{
    Thread thread(MainThreadSize, [this]{
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
    _eventThread.swap(thread);
    _eventThread.detach();
}

TaskManager::~TaskManager()
{
    _terminating = true;
    readyToExecuteNextTask();
    _eventThread.join();
}

void TaskManager::readyToExecuteNextTask()
{
    Lock lock(_eventMutex);
    _eventCondition.notify(true);
}

void TaskManager::runLoop()
{
    // The main thread has to keep running
    while(1) {
        // Check for key input
//        char* buffer = nullptr;
//        size_t size = 0;
//        getline(&buffer, &size, stdin);
//        system()->receivedLine(buffer);
        this_thread::sleep_for(MainLoopSleepDelay);
    }
}
void TaskManager::run(TaskBase* newTask)
{
    {
        Lock lock(_mutex);
        _readyList.push_back(newTask);
    }
    readyToExecuteNextTask();
}

void TaskManager::terminate(TaskBase* task)
{
    {
        Lock lock(_mutex);
        _readyList.remove(task);
    }
    _waitEventList.remove(task);
}

void TaskManager::executeNextTask()
{
    if (!ready()) {
        return;
    }
    
    TaskBase* task;
    {
        Lock lock(_mutex);
        task = _readyList.front();
        _readyList.erase(_readyList.begin());
    }
    
    CallReturnValue returnValue = task->execute();
    
    if (returnValue.isDelay()) {
        Duration duration = returnValue.delay();
        printf("***** Delaying %s\n", duration.toString().c_str());
        Thread(DelayThreadSize, [this, task, duration] {
            duration.sleep();
            {
                Lock lock(_mutex);
                _readyList.push_back(task);
            }
            readyToExecuteNextTask();
        }).detach();
    } else if (returnValue.isYield()) {
        {
            Lock lock(_mutex);
            _readyList.push_back(task);
        }
    } else if (returnValue.isTerminated()) {
        task->finish();
    } else if (returnValue.isFinished()) {
        task->finish();
    } else if (returnValue.isWaitForEvent()) {
        _waitEventList.push_back(task);
    }
}
