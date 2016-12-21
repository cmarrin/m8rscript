/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
	  
    - Redistributions in binary form must reproduce the above copyright 
	  notice, this list of conditions and the following disclaimer in the 
	  documentation and/or other materials provided with the distribution.
	  
    - Neither the name of the <ORGANIZATION> nor the names of its 
	  contributors may be used to endorse or promote products derived from 
	  this software without specific prior written permission.
	  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "TaskManager.h"

#include "SystemInterface.h"
#include <cassert>

using namespace m8r;

TaskManager* TaskManager::_sharedTaskManager = nullptr;

void TaskManager::runTask(Task* newTask, int32_t delay)
{
    int32_t now = msNow();
    if (delay > 6000000) {
        delay = 6000000;
    } else if (delay < 5) {
        delay = 0;
    }
    
    newTask->_msSet = delay;
    int32_t msTimeToFire = now + delay;
    newTask->_msTimeToFire = msTimeToFire;
    
    Task* prev = nullptr;
    for (Task* task = _head; ; task = task->_next) {
        if (!task) {
            // Placing a new task in an empty list, need to schedule
            _head = newTask;
            break;
        }
        if (msTimeToFire < task->_msTimeToFire) {
            if (prev) {
                // Placing a new task in a list that already has tasks, assume an event is already scheduled
                newTask->_next = prev->_next;
                prev->_next = newTask;
                return;
            } else {
                // Placing a new task at the head of an existing list, need to schedule
                newTask->_next = _head;
                _head = newTask;
                break;
            }
        }
        if (!task->_next) {
            // Placing new task at end of list, assume an event is already scheduled
            task->_next = newTask;
            return;
        }
        prev = task;
    }
    prepareForNextEvent();
}

void TaskManager::fireEvent()
{
    assert(_head);
    
    Task* task = _head;
    _head = task->_next;
    task->_next = nullptr;
    
    task->execute();
    
    if (task->_repeating) {
        runTask(task, task->_msSet);
    } else {
        prepareForNextEvent();
    }
}

void TaskManager::prepareForNextEvent()
{
    if (!_head) {
        return;
    }
    postEvent();
}

int32_t TaskManager::msNow()
{
    return static_cast<int32_t>(SystemInterface::currentMicroseconds() / 1000);
}

