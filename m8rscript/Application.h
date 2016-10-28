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

#include "ExecutionUnit.h"
#include "TaskManager.h"
#include <functional>

//#ifdef __APPLE__
//#else
//extern "C" {
//#include <osapi.h>
//#include <user_interface.h>
//}
//#endif

namespace m8r {

static const char* MainFileName = "main";

class SystemInterface;
class Program;
class Error;
class ExecutionUnit;

class Application {
public:
    Application(SystemInterface*);
    
    bool load(Error&, const char* name = nullptr);
    void run(std::function<void()>);
    
    Program* program() const { return _program; }
    
    enum class NameValidationType { Ok, BadLength, InvalidChar };
    static NameValidationType validateFileName(const char* name);
    static NameValidationType validateBonjourName(const char* name);

private:
    class MyRunTask : public Task {
    public:
        MyRunTask(SystemInterface* system) : _eu(system) { }
        
        void run(Program* program, std::function<void()> function)
        {
            _function = function;
            _eu.startExecution(program);
            runOnce();
        }

    private:
        virtual bool execute() override;
        
        ExecutionUnit _eu;
        std::function<void()> _function;
    };

    SystemInterface* _system;
    Program* _program = nullptr;
    MyRunTask _runTask;
    
//#ifdef __APPLE__
//#else
//    static constexpr uint32_t ExecutionTaskPrio = 0;
//    static constexpr uint32_t ExecutionTaskQueueLen = 1;
//
//    static void executionTask(os_event_t *);
//    static void executionTimerTick(void* data);
//
//    os_timer_t _executionTimer;
//    os_event_t _executionTaskQueue[ExecutionTaskQueueLen];
//#endif
};
    
}
