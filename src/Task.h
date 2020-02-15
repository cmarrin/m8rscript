/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
#include "SystemInterface.h"
#include "TaskManager.h"
#include <cstdint>
#include <functional>

namespace m8r {

class ExecutionUnit;
class String;

class TaskBase : public OrderedList<TaskBase, Time>::Item {
    friend class TaskManager;
    
public:
    using FinishCallback = std::function<void(TaskBase*)>;
    
    enum class Behavior { Once, Repeating };
    
    ~TaskBase()
    {
        system()->taskManager()->terminate(this);
    }
    
    void run(const FinishCallback& cb = nullptr, Duration duration = 0_sec, Behavior behavior = Behavior::Once)
    {
        _finishCB = cb;
        _duration = (behavior == Behavior::Repeating) ? duration : 0_sec;
        system()->taskManager()->yield(this, duration);
    }

    Duration duration() const { return _duration; }
    
    void yield() { system()->taskManager()->yield(this); }
    void terminate() { system()->taskManager()->terminate(this); }

    Error error() const { return _error; }

protected:
    TaskBase() { }
    
    Error _error = Error::Code::OK;

private:
    void finish();
    
    virtual CallReturnValue execute() = 0;
    
    FinishCallback _finishCB;
    Duration _duration = 0_sec; // If any value other than 0, this is a repeating task
};

class Task : public NativeObject, public TaskBase {
public:
    Task();
    
    virtual ~Task();
    
    bool init(const Stream&);
    bool init(const char* filename);
    
    void receivedData(const String& data, KeyAction action);

    void setConsolePrintFunction(std::function<void(const String&)> f);
    void setConsoleListener(Value func);

    const ExecutionUnit* eu() const { return _eu.get(); }
    
private:
    virtual CallReturnValue execute() override;

    Mad<ExecutionUnit> _eu;    
};

class TaskProto : public StaticObject {
public:
    TaskProto(bool isTimer);

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue run(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
private:
    bool _isTimer;
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
