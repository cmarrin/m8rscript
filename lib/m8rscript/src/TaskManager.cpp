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
#include "Timer.h"
#include <cassert>

using namespace m8r;

static Duration MainLoopSleepDelay = 50_ms;
static Duration MaxTaskTimeSlice = 50_ms;

TaskManager::TaskManager()
{
    _timeSliceTimer = Mad<Timer>::create();
    _timeSliceTimer->init(MaxTaskTimeSlice, Timer::Behavior::Once, [this](Timer*) {
        requestYield();
    });
}

TaskManager::~TaskManager()
{
    _terminating = true;
    readyToExecuteNextTask();
}

void TaskManager::readyToExecuteNextTask()
{
    requestYield();
}

void TaskManager::startTimeSliceTimer()
{
    _timeSliceTimer->start();
}

void TaskManager::requestYield()
{
    if (_currentTask) {
        _currentTask->requestYield();
    }
}

void TaskManager::runOneIteration()
{
    // See if there's anything to be done
    executeNextTask();
}
void TaskManager::run(TaskBase* newTask)
{
    {
        _list.push_back(newTask);
        newTask->setState(Task::State::Ready);
    }
    readyToExecuteNextTask();
}

void TaskManager::terminate(TaskBase* task)
{
    {
        _list.remove(task);
        task->setState(Task::State::Terminated);
    }
}

bool TaskManager::executeNextTask()
{
    // Check timers
    Time currentTime = Time::now();
    
    while (1) {
        if (!_timerList.empty() && _timerList.front()->timeToFire() >= currentTime) {
            auto timer = _timerList.front();
            _timerList.erase(_timerList.begin());
            timer->fire();
        } else {
            break;
        }
    }
    
    // Find the next executable task
    auto it = std::find_if(_list.begin(), _list.end(), [](TaskBase* task) {
        return task->readyToRun();
    });

    if (it == _list.end()) {
        return false;
    }
    
    _currentTask = *it;
    
    if (_currentTask->state() == Task::State::Ready) {
        // Move the task to the end of the list to let other ready tasks run
        _list.erase(it);
        _list.push_back(_currentTask);
    }

    startTimeSliceTimer();
    CallReturnValue returnValue = _currentTask->execute();
    
    if (returnValue.isYield()) {
        _currentTask->setState(Task::State::Ready);
    } else if (returnValue.isTerminated() || returnValue.isFinished()) {
        _currentTask->setState(Task::State::Terminated);
        _list.remove(_currentTask);
        _currentTask->finish();
        _currentTask = nullptr;
    } else if (returnValue.isWaitForEvent()) {
        _currentTask->setState(Task::State::WaitingForEvent);
    } else if (returnValue.isDelay()) {
        _currentTask->setState(Task::State::Delaying);
    }
    
    return true;
}

void TaskManager::addTimer(Timer* timer)
{
    system()->stopTimer();
    _timerList.push_back(timer);
    std::sort(_timerList.begin(), _timerList.end());
    restartTimer();
}

void TaskManager::removeTimer(Timer* timer)
{
    system()->stopTimer();
    _timerList.remove(timer);
    restartTimer();
}

void TaskManager::restartTimer()
{
    if (!_timerList.empty()) {
        system()->startTimer(_timerList[0]->timeToFire() - Time::now(), [this] {
            requestYield();
        });
    }
}
