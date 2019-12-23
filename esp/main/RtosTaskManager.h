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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace m8r {

class RtosTaskManager : public TaskManager {
public:
    RtosTaskManager();
    virtual ~RtosTaskManager();
    
private:
    // The ESP handlles it's own runloop, we can just return here
    virtual void runLoop() { }

    virtual void readyToExecuteNextTask() override;
    
    static constexpr UBaseType_t Priority = tskIDLE_PRIORITY;
    static constexpr configSTACK_DEPTH_TYPE StackSize = 4096;

    static void executionTask(void*);

    TaskHandle_t _task = nullptr;

};

}
