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
static Duration TaskPollingRate = 50_ms;


void TaskManager::run(TaskBase* newTask)
{
    _readyList.push_back(newTask);
    
    readyToExecuteNextTask();
}

void TaskManager::terminate(TaskBase* task)
{
    _readyList.remove(task);
    _waitEventList.remove(task);
}

void TaskManager::executeNextTask()
{
    if (!ready()) {
        return;
    }
    
    TaskBase* task = _readyList.front();
    _readyList.erase(_readyList.begin());
    
    CallReturnValue returnValue = task->execute();
    
    if (returnValue.isDelay()) {
        Duration duration = returnValue.delay();
        Thread([this, task, duration] {
            duration.sleep();
            _readyList.push_back(task);
            readyToExecuteNextTask();
        }).detach();
    } else if (returnValue.isYield()) {
        _readyList.push_back(task);
    } else if (returnValue.isTerminated()) {
        task->finish();
    } else if (returnValue.isFinished()) {
        task->finish();
    } else if (returnValue.isWaitForEvent()) {
        _waitEventList.push_back(task);
    }
}
