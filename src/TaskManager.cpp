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
#include "Task.h"
#include <cassert>

using namespace m8r;

static Duration MaxTaskDelay = Duration(6000000, Duration::Units::ms);
static Duration MinTaskDelay = Duration(1, Duration::Units::ms);
static Duration TaskPollingRate = 50_ms;


void TaskManager::yield(TaskBase* newTask, Duration delay)
{
    Time now = Time::now();
    if (delay > MaxTaskDelay) {
        delay = MaxTaskDelay;
    } else if (delay < MinTaskDelay) {
        delay = Duration();
    }
    
    Time timeToFire = now + delay;
    
    auto prev = _list.before_begin();
    for (auto it = _list.begin(); ; ++it) {
        if (it == _list.end() || timeToFire < it->first) {
            _list.emplace_after(prev, timeToFire, newTask);
            break;
        }
        prev = it;
    }

    readyToExecuteNextTask();
}

void TaskManager::terminate(TaskBase* task)
{
    auto prev = _list.before_begin();
    for (auto it = _list.begin(); it != _list.end(); ++it) {
        if (it->second == task) {
            _list.erase_after(it);
            break;
        }
        prev = it;
    }
}

void TaskManager::executeNextTask()
{
    if (_list.empty() || !_list.front().second) {
        return;
    }
    
    TaskBase* task = _list.front().second;
    CallReturnValue returnValue = task->execute();
    
    if (returnValue.isMsDelay()) {
        yield(task, returnValue.msDelay());
    } else if (returnValue.isYield()) {
        yield(task);
    } else if (returnValue.isFinished() || returnValue.isTerminated()) {
        _list.pop_front();
    } else if (returnValue.isWaitForEvent()) {
        yield(task, TaskPollingRate);
    }
}

Time TaskManager::nextTimeToFire() const
{
    return _list.empty() ? Time::longestTime() : _list.front().first;
}
