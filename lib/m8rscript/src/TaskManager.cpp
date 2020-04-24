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

static Duration MaxTaskTimeSlice = 50ms;

TaskManager::TaskManager()
{
    _timeSliceTimer = Timer::create(MaxTaskTimeSlice, Timer::Behavior::Once, [this](Timer*)
    {
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

void TaskManager::stopTimeSliceTimer()
{
    _timeSliceTimer->stop();
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
void TaskManager::run(const std::shared_ptr<TaskBase>& newTask, FinishCallback cb)
{
    {
        newTask->_finishCB = cb;
        _list.push_back(newTask);
        newTask->setState(Task::State::Ready);
    }
    readyToExecuteNextTask();
}

void TaskManager::terminate(const std::shared_ptr<TaskBase>& task)
{
    {
        _list.remove(task);
        task->setState(Task::State::Terminated);
    }
}

bool TaskManager::executeNextTask()
{
    // Check timers
    if (_timerList.empty() && _list.empty()) {
        return false;
    }
    
    Time currentTime = Time::now();
    
    while (1) {
        if (!_timerList.empty() && _timerList.front()->timeToFire() <= currentTime) {
            auto timer = _timerList.front();
            _timerList.erase(_timerList.begin());
            DBG_TIMERS("firing timer: duration=%s, now=%s, timeToFire=%s", 
                        timer->duration().toString().c_str(), 
                        currentTime.toString().c_str(), 
                        timer->timeToFire().toString().c_str());
            timer->fire();
        } else {
            break;
        }
    }
    
    // Find the next executable task
    auto it = std::find_if(_list.begin(), _list.end(), [](std::shared_ptr<TaskBase> task) {
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
    stopTimeSliceTimer();
    
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
    system()->stopTimer(_timerId);
    _timerId = -1;
    _timerList.push_back(timer);
    std::sort(_timerList.begin(), _timerList.end(), [](const Timer* a, const Timer* b) {
        return a->timeToFire() < b->timeToFire();
    });
    restartTimer();
}

void TaskManager::removeTimer(Timer* timer)
{
    system()->stopTimer(_timerId);
    _timerList.remove(timer);
    restartTimer();
}

void TaskManager::restartTimer()
{
    if (!_timerList.empty()) {
        DBG_TIMERS("restartTimer: duration=%s", (_timerList[0]->timeToFire() - Time::now()).toString().c_str());
        _timerId = system()->startTimer(_timerList[0]->timeToFire() - Time::now(), [this] {
            requestYield();
        });
    }
}
