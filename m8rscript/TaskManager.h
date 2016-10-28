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

#pragma once

#include <cstdint>

namespace m8r {

class Task;

class TaskManager {
    friend class Task;
    
protected:
    static constexpr uint8_t MaxTasks = 8;
    
    static TaskManager* sharedTaskManager();
    
    TaskManager() { }
    virtual ~TaskManager() { }
    
    virtual void runTask(Task*, int32_t delay);
    
    void fireEvent();

private:
    void prepareForNextEvent();
    
    // Stop any currently running timer
    virtual void stopTimer() = 0;
    
    // Start a timer, after ms call postEvent
    virtual void startTimer(int32_t ms) = 0;
    
    // Post an event now. When event occurs, call fireEvent
    virtual void postEvent() = 0;
    
    Task* _head = nullptr;
    bool _eventPosted = false;

    static TaskManager* _sharedTaskManager;
};

class Task {
    friend class TaskManager;
    
public:
    virtual ~Task() { }
    
    void runOnce(int32_t delay = 0)
    {
        _repeating = false;
        TaskManager::sharedTaskManager()->runTask(this, delay);
    }
    void runRepeating(int32_t delay)
    {
        _repeating = true;
        TaskManager::sharedTaskManager()->runTask(this, delay);
    }
    
    virtual bool execute() { return true; }
    
private:
    int32_t _msSet = 0;
    int32_t _msTimeToFire = -1;
    Task* _next = nullptr;
    bool _repeating;
};

}
