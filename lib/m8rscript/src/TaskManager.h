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
#include "Timer.h"
#include <cstdint>
#include <memory>

namespace m8r {

class Task;

class TaskManager {
    friend class SystemInterface;
    friend class Task;
    friend class ExecutionUnit;
    friend class Timer;

public:
    using FinishCallback = std::function<void(Task*)>;
    
    void run(const std::shared_ptr<Task>&, FinishCallback);
    void terminate(const std::shared_ptr<Task>&);
    
protected:
    static constexpr uint8_t MaxTasks = 8;

    TaskManager();
    ~TaskManager();
    
    bool executeNextTask();
    
    void addTimer(Timer*);
    void removeTimer(Timer*);

private:
    bool runOneIteration();

    void readyToExecuteNextTask();
    void startTimeSliceTimer();
    void stopTimeSliceTimer();
    void requestYield();
    
    void restartTimer();
    
    Vector<std::shared_ptr<Task>> _list;
    Vector<Timer*> _timerList;
    int8_t _timerId = -1;
    
    std::shared_ptr<Task> _currentTask;

    std::shared_ptr<Timer> _timeSliceTimer;
    bool _terminating = false;
};

}
