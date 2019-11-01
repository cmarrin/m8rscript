/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "EspTaskManager.h"

#include "Esp.h"
#include "SystemInterface.h"

using namespace m8r;

EspTaskManager::EspTaskManager()
{
    system_os_task(executionTask, ExecutionTaskPrio, _executionTaskQueue, ExecutionTaskQueueLen);
    os_timer_disarm(&_executionTimer);
    os_timer_setfn(&_executionTimer, (os_timer_func_t*) &executionTimerTick, this);
}

EspTaskManager::~EspTaskManager()
{
}

void EspTaskManager::readyToExecuteNextTask()
{
    system_os_post(ExecutionTaskPrio, 0, reinterpret_cast<uint32_t>(this));
}

void EspTaskManager::executionTask(os_event_t *event)
{
    EspTaskManager* taskManager = reinterpret_cast<EspTaskManager*>(event->par);
    if (taskManager->empty()) {
        return;
    }

    Time now = Time::now();
    Duration durationToNextEvent = taskManager->nextTimeToFire() - now;
    if (durationToNextEvent <= 5_ms) {
        taskManager->executeNextTask();
    } else {
        os_timer_arm(&taskManager->_executionTimer, durationToNextEvent.ms(), false);
    }
}

void EspTaskManager::executionTimerTick(void* data)
{
    EspTaskManager* taskManager = reinterpret_cast<EspTaskManager*>(data);
    taskManager->readyToExecuteNextTask();
}
