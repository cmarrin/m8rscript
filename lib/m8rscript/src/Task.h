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

class TaskBase {
    friend class TaskManager;
    
public:
    enum class State { Ready, WaitingForEvent, Delaying, Terminated };
    
    virtual ~TaskBase() { }
    
    Error error() const { return _error; }
    
    virtual bool readyToRun() const { return state() == State::Ready; }
    virtual void requestYield() const { }
    
    virtual void receivedData(const String& data, KeyAction action) { }
    virtual void setConsolePrintFunction(std::function<void(const String&)> f) { }
    
    virtual void print(const char* s) const
    {
        system()->print(s);
    }

#ifndef NDEBUG
    const String& name() const { return _name; }
#endif

protected:
    TaskBase() { }
    
    State state() const { return _state; }
    void setState(State state) { _state = state; }

    Error _error = Error::Code::OK;

#ifndef NDEBUG
    String _name;
#endif

private:
    void finish() { if (_finishCB) _finishCB(this); }
    
    virtual CallReturnValue execute() = 0;    

    TaskManager::FinishCallback _finishCB;
    
    State _state = State::Ready;
};

class Task : public NativeObject, public TaskBase {
public:
    Task();
    virtual ~Task();
    
    static std::shared_ptr<Task> create() { return std::make_shared<Task>(); }
    
    bool load(const Stream&);
    bool load(const char* filename);
    
    virtual void receivedData(const String& data, KeyAction action) override;

    virtual void setConsolePrintFunction(std::function<void(const String&)> f) override;
    void setConsoleListener(Value func);

    virtual void print(const char* s) const override;

    const ExecutionUnit* eu() const { return _eu.get(); }
    
    virtual bool readyToRun() const override;
    virtual void requestYield() const override;

private:
    virtual CallReturnValue execute() override;

    Mad<ExecutionUnit> _eu;    
};

class TaskProto : public StaticObject {
public:
    TaskProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue run(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
