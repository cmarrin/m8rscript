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
#include <cassert>

using namespace m8r;

static Duration MaxTaskDelay = Duration(6000000, Duration::Units::ms);
static Duration MinTaskDelay = Duration(1, Duration::Units::ms);
static Duration TaskPollingRate = 50_ms;


void TaskManager::yield(TaskBase* newTask, Duration delay)
{
    Time now = Time::now();
    if (delay > MaxTaskDelay) {
        delay = MaxTaskDelay;
    } else if (delay < MinTaskDelay) {
        delay = Duration();
    }
    
    Time timeToFire = now + delay;
    
    _list.insert(newTask, timeToFire);
    
    readyToExecuteNextTask();
}

void TaskManager::terminate(TaskBase* task)
{
    _list.remove(task);
}

void TaskManager::executeNextTask()
{
    if (_list.empty()) {
        return;
    }
    
    TaskBase* task = _list.front();
    _list.pop_front();
    
    CallReturnValue returnValue = task->execute();
    
    if (returnValue.isMsDelay()) {
        yield(task, returnValue.msDelay());
    } else if (returnValue.isYield()) {
        yield(task);
    } else if (returnValue.isTerminated()) {
        task->finish();
    } else if (returnValue.isFinished()) {
        task->finish();
    } else if (returnValue.isWaitForEvent()) {
        yield(task, TaskPollingRate);
    }
}

Time TaskManager::nextTimeToFire() const
{
    return _list.empty() ? Time::longestTime() : _list.front()->key();
}
