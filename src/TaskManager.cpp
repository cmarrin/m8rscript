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
#include "Thread.h"
#include <cassert>

using namespace m8r;

static Duration MainLoopSleepDelay = 50_ms;
static constexpr uint32_t MainThreadSize = 4096;

static Duration MaxTaskTimeSlice = 50_ms;
static constexpr uint32_t TimeSliceThreadSize = 1024;

TaskManager::TaskManager()
{
    Thread mainThread(MainThreadSize, [this]{
        while (true) {
            if (!executeNextTask()) {
                Lock lock(_mainMutex);
                _mainCondition.wait(lock);
                if (_terminating) {
                    break;
                }
            }
        }
    });
    _mainThread.swap(mainThread);

    // Setup a thread that will yield after MaxTaskTimeSlice seconds
    Thread timeSliceThread(TimeSliceThreadSize, [this] {
        while (!_terminating) {
            {
                Lock lock(_timeSliceMutex);
                _timeSliceCondition.wait(lock);
            }
            MaxTaskTimeSlice.sleep();
            requestYield();
        }
    });
    _timeSliceThread.swap(timeSliceThread);
}

TaskManager::~TaskManager()
{
    _terminating = true;
    readyToExecuteNextTask();
    _mainThread.join();
}

void TaskManager::readyToExecuteNextTask()
{
    Lock lock(_mainMutex);
    _mainCondition.notify(true);
}

void TaskManager::startTimeSliceTimer()
{
    Lock lock(_timeSliceMutex);
    _timeSliceCondition.notify(true);
}

void TaskManager::requestYield()
{
    Lock lock(_mainMutex);
    if (_currentTask) {
        _currentTask->requestYield();
    }
}

void TaskManager::runLoop()
{
    // The main thread has to keep running
    while(1) {
        // Check for key input
//        char* buffer = nullptr;
//        size_t size = 0;
//        getline(&buffer, &size, stdin);
//        system()->receivedLine(buffer);
        this_thread::sleep_for(MainLoopSleepDelay);
    }
}
void TaskManager::run(TaskBase* newTask)
{
    {
        Lock lock(_mainMutex);
        _list.push_back(newTask);
        newTask->setState(Task::State::Ready);
    }
    readyToExecuteNextTask();
}

void TaskManager::terminate(TaskBase* task)
{
    {
        Lock lock(_mainMutex);
        _list.remove(task);
        task->setState(Task::State::Terminated);
    }
}

bool TaskManager::executeNextTask()
{
    {
        Lock lock(_mainMutex);
        
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
