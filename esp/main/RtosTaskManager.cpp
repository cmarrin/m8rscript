/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "RtosTaskManager.h"

using namespace m8r;

static Duration NextEventDelay = 50_ms;
static Duration MinEventDuration = 5_ms;

RtosTaskManager::RtosTaskManager()
{
    xTaskCreate(executionTask, "Interpreter", StackSize, this, Priority, &_task);
}

RtosTaskManager::~RtosTaskManager()
{
    // TODO: Destroy task
    _terminating = true;
}

void RtosTaskManager::readyToExecuteNextTask()
{
    xTaskNotify(_task, 0, eNoAction);
}

void RtosTaskManager::executionTask(void* param)
{
    RtosTaskManager* taskManager = reinterpret_cast<RtosTaskManager*>(param);
    while (1) {
        Duration durationToNextEvent = NextEventDelay;

        if (!taskManager->ready()) {
            xTaskNotifyWait(0, 0, nullptr, 50000 / portTICK_PERIOD_MS);
            if (taskManager->_terminating) {
                break;
            }
        }
        
        taskManager->executeNextTask();
    }
}

void RtosTaskManager::runLoop()
{
    while (1) {
        vTaskDelay(NextEventDelay.ms() / portTICK_RATE_MS);
    }
}
