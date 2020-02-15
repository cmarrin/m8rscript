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
    
    virtual ~TaskBase()
    {
        system()->taskManager()->terminate(this);
    }
    
    void yield(Duration duration) { system()->taskManager()->yield(this, duration); }
    void terminate() { system()->taskManager()->terminate(this); }
    
    virtual Duration duration() const { return 0_sec; }

    Error error() const { return _error; }

protected:
    TaskBase() { }
    
    Error _error = Error::Code::OK;

private:
    virtual void finish() = 0;
    
    virtual CallReturnValue execute() = 0;
};

class Task : public NativeObject, public TaskBase {
public:
    Task();
    
    virtual ~Task();
    
    bool init(const Stream&);
    bool init(const char* filename);
    
    void run(const FinishCallback& cb = nullptr)
    {
        _finishCB = cb;
        yield(0_sec);
    }

    void receivedData(const String& data, KeyAction action);

    void setConsolePrintFunction(std::function<void(const String&)> f);
    void setConsoleListener(Value func);

    const ExecutionUnit* eu() const { return _eu.get(); }
    
    virtual void finish() override { if (_finishCB) _finishCB(this); }

private:
    virtual CallReturnValue execute() override;

    Mad<ExecutionUnit> _eu;    
    
    FinishCallback _finishCB;
};

class TaskProto : public StaticObject {
public:
    TaskProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue run(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
