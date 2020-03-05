/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "SystemInterface.h"
#include "SystemTime.h"
#include <cstdint>

namespace m8r {

class TaskBase;

class TaskManager {
    friend class SystemInterface;
    friend class TaskBase;

public:

protected:
    static constexpr uint8_t MaxTasks = 8;

    TaskManager() { }
    virtual ~TaskManager() { }
    
    void run(TaskBase*);
    
    void terminate(TaskBase*);
    
    void executeNextTask();
    
    bool ready() const { return !_readyList.empty(); }
    
private:
    virtual void runLoop() = 0;

    // Post an event now. When event occurs, call fireEvent
    virtual void readyToExecuteNextTask() = 0;
    
    Vector<TaskBase*> _readyList;
    Vector<TaskBase*> _waitEventList;
};

}
