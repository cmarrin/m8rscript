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

static Duration MaxTaskDelay = Duration(6000000, Duration::Units::ms);
static Duration MinTaskDelay = Duration(1, Duration::Units::ms);
static Duration MainLoopSleepDelay = 50_ms;
static constexpr uint32_t MainThreadSize = 4096;
static constexpr uint32_t DelayThreadSize = 1024;

TaskManager::TaskManager()
{
    Thread thread(MainThreadSize, [this]{
        while (true) {
            if (!executeNextTask()) {
                Lock lock(_mutex);
                _eventCondition.wait(lock);
                if (_terminating) {
                    break;
                }
            }
        }
    });
    _eventThread.swap(thread);
    _eventThread.detach();
}

TaskManager::~TaskManager()
{
    _terminating = true;
    readyToExecuteNextTask();
    _eventThread.join();
}

void TaskManager::readyToExecuteNextTask()
{
    Lock lock(_mutex);
    _eventCondition.notify(true);
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
        Lock lock(_mutex);
        _list.push_back(newTask);
        printf("run:State::Ready\n");
        newTask->setState(Task::State::Ready);
    }
    readyToExecuteNextTask();
}

void TaskManager::terminate(TaskBase* task)
{
    {
        Lock lock(_mutex);
        _list.remove(task);
        printf("terminate:State::Terminated\n");
        task->setState(Task::State::Terminated);
    }
}

bool TaskManager::executeNextTask()
{
    printf("***** executeNextTask: enter\n");
    TaskBase* task;
    {
        Lock lock(_mutex);
        
        // Find the next executable task
        auto it = std::find_if(_list.begin(), _list.end(), [](TaskBase* task) {
            return task->state() == Task::State::Ready || task->hasEvents();
        });

        if (it == _list.end()) {
            printf("***** executeNextTask: exit false\n");
            return false;
        }
        
        task = *it;
        
        if (task->state() == Task::State::Ready) {
            // Move the task to the end of the list to let other ready tasks run
            _list.erase(it);
            _list.push_back(task);
        }
    }
    
    if (task->state() != Task::State::Ready) {
        printf("***** executeNextTask: task is %sready\n", (task->state() == Task::State::Ready) ? "" : "not ");
    }
    
    CallReturnValue returnValue = task->execute();
    
    if (returnValue.isDelay()) {
        Duration duration = returnValue.delay();
        printf("delay:State::WaitForEvent\n");
        task->setState(Task::State::WaitingForEvent);
        
        Thread(DelayThreadSize, [this, task, duration] {
            duration.sleep();
            {
                Lock lock(_mutex);
                printf("delay over:State::Ready\n");
                task->setState(Task::State::Ready);
            }
            readyToExecuteNextTask();
        }).detach();
    } else if (returnValue.isYield()) {
        // Yield is returned if we are still ready and want
        // to run again or if we just processed events.
        // In either case leave the task in the same state.
    } else if (returnValue.isTerminated()) {
        printf("isTerminated:State::Terminated\n");
        task->setState(Task::State::Terminated);
        _list.remove(task);
        task->finish();
    } else if (returnValue.isFinished()) {
        printf("isFinished:State::Terminated\n");
        task->setState(Task::State::Terminated);
        _list.remove(task);
        task->finish();
    } else if (returnValue.isWaitForEvent()) {
        printf("isWaitForEvent:State::WaitingForEvent\n");
        task->setState(Task::State::WaitingForEvent);
    }
    
    return true;
}
