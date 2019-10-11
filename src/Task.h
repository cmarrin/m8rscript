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
#include "SystemInterface.h"
#include "TaskManager.h"
#include <cstdint>

namespace m8r {

class TaskBase {
    friend class TaskManager;
    
public:
    virtual ~TaskBase()
    {
        system()->taskManager()->terminate(this);
    }
    
    void run(Duration duration = 0_sec) { system()->taskManager()->yield(this, duration); }
    void yield() { system()->taskManager()->yield(this); }
    void terminate() { system()->taskManager()->terminate(this); }

protected:
    TaskBase() { }
    
private:
    virtual CallReturnValue execute() = 0;
};

class Task : public TaskBase {
public:
    Task() { }
    
    Task(const char* filename);
    
    Task(const std::shared_ptr<Program>& program)
    {
        _eu.startExecution(program);
    }
    
    Error error() const { return _error; }

private:
    virtual CallReturnValue execute() { return _eu.continueExecution(); }

    ExecutionUnit _eu;
    Error _error;
};

class TaskProto : public ObjectFactory {
public:
    TaskProto(Program*);

private:
    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
    NativeFunction _constructor;
};

class NativeTask : public TaskBase {
public:
    using Function = std::function<CallReturnValue()>;
    
    NativeTask() { }
    NativeTask(Function f) : _f(f) { }

protected:
private:
    virtual CallReturnValue execute() { return _f(); }

    Function _f;
};

}
