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
#include "Thread.h"

namespace m8r {

class MacTaskManager : public TaskManager {
public:
    MacTaskManager();
    virtual ~MacTaskManager();
    
private:
    virtual void runLoop() override;
    
    virtual void readyToExecuteNextTask() override;
    
    Thread _eventThread;
    Condition _eventCondition;
    Mutex _eventMutex;
    bool _terminating = false;
};

}
