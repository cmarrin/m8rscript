/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "TaskManager.h"

#include "Esp.h"

extern "C" {
#include <osapi.h>
#include "user_interface.h"
}

namespace m8r {

class EspTaskManager : public TaskManager {
public:
    EspTaskManager();
    virtual ~EspTaskManager();
    
    virtual void lock() override { noInterrupts(); }
    virtual void unlock() override { interrupts(); }

private:
    // The ESP handlles it's own runloop, we can just return here
    virtual void runLoop() { }

    virtual void readyToExecuteNextTask() override;
    
    static constexpr uint32_t ExecutionTaskPrio = 0;
    static constexpr uint32_t ExecutionTaskQueueLen = 1;

    static void executionTask(os_event_t*);
    static void executionTimerTick(void* data);

    os_timer_t _executionTimer;
    os_event_t _executionTaskQueue[ExecutionTaskQueueLen];
};

}
