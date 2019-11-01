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
#include <forward_list>

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
    
    virtual void yield(TaskBase*, Duration = Duration());
    
    void terminate(TaskBase*);
    
    void executeNextTask();
    
    bool empty() const { return _list.empty(); }
    Time nextTimeToFire() const;
    
private:
    virtual void runLoop() = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;
    
    // Post an event now. When event occurs, call fireEvent
    virtual void readyToExecuteNextTask() = 0;
    
    List<TaskBase, Time> _list;
    bool _eventPosted = false;
};

}
