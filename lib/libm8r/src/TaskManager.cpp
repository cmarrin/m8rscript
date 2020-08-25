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
    _timeSliceTimer.setCallback([this](Timer*)
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
    _timeSliceTimer.start(MaxTaskTimeSlice);
}

void TaskManager::stopTimeSliceTimer()
{
    _timeSliceTimer.stop();
}

void TaskManager::requestYield()
{
    if (_currentTask) {
        _currentTask->requestYield();
    }
}

void TaskManager::run(const SharedPtr<Task>& newTask, FinishCallback cb)
{
    {
        newTask->_finishCB = cb;
        _list.push_back(newTask);
        newTask->setState(Task::State::Ready);
    }
    readyToExecuteNextTask();
}

void TaskManager::terminate(const SharedPtr<Task>& task)
{
    {
        _list.remove(task);
        task->setState(Task::State::Terminated);
    }
}

bool TaskManager::runOneIteration()
{
    if (_list.empty()) {
        return false;
    }
    
    // Find the next executable task
    auto it = std::find_if(_list.begin(), _list.end(), [](SharedPtr<Task> task) {
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
    } else if (returnValue.isTerminated() || returnValue.isFinished() || returnValue.isError()) {
        if (returnValue.isError()) {
            Error error = returnValue.error();
            String errorString = error.formatError(_currentTask->runtimeErrorString());
            _currentTask->print(errorString.c_str());
        }
        
        _currentTask->setState(Task::State::Terminated);
        _list.remove(_currentTask);
        _currentTask->finish();
        _currentTask.reset();
    } else if (returnValue.isWaitForEvent()) {
        _currentTask->setState(Task::State::WaitingForEvent);
    } else if (returnValue.isDelay()) {
        _currentTask->setState(Task::State::Delaying);
    }
    return true;
}
