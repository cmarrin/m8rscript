/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
#include "SystemTime.h"
#include <cstdint>

namespace m8r {

class TaskBase;
class Timer;

class TaskManager {
    friend class SystemInterface;
    friend class TaskBase;
    friend class ExecutionUnit;
    friend class Timer;

public:
    using FinishCallback = std::function<void(TaskBase*)>;
    
    void run(TaskBase*, FinishCallback);
    void terminate(TaskBase*);
    
protected:
    static constexpr uint8_t MaxTasks = 8;

    TaskManager();
    ~TaskManager();
    
    bool executeNextTask();
    
    void addTimer(Timer*);
    void removeTimer(Timer*);

private:
    void runOneIteration();

    void readyToExecuteNextTask();
    void startTimeSliceTimer();
    void stopTimeSliceTimer();
    void requestYield();
    
    void restartTimer();
    
    Vector<TaskBase*> _list;
    Vector<Timer*> _timerList;
    
    TaskBase* _currentTask = nullptr;

    Mad<Timer> _timeSliceTimer;
    bool _terminating = false;
};

}
