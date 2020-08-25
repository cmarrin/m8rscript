/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Error.h"
#include "Executable.h"
#include "TaskManager.h"
#include <functional>

namespace m8r {

class Stream;
class String;

class Task : public Shared {
    friend class TaskManager;
    
public:        
    enum class State { Ready, WaitingForEvent, Delaying, Terminated };
    
    Task() { }
    
    ~Task();
    
    bool load(const SharedPtr<Executable>& exec)
    {
        _executable = exec;
        return true;
    }

    bool load(const Stream&, const String& type);
    bool load(const char* filename);

    Error error() const { return _error; }
    
    bool readyToRun() const { return state() == State::Ready || _executable->readyToRun(); }
    void requestYield() const { _executable->requestYield(); }
    
    void receivedData(const String& data, KeyAction action) { _executable->receivedData(data, action); }
    
    void print(const char* s) const;
    
    void setConsolePrintFunction(const std::function<void(const String&)>& f)
    {
        if (_executable) {
            _executable->setConsolePrintFunction(f);
        }
    }
    
    const char* runtimeErrorString() const { return _executable ? _executable->runtimeErrorString() : "unknown"; }

#ifndef NDEBUG
    const String& name() const { return _name; }
#endif

protected:
    State state() const { return _state; }
    void setState(State state) { _state = state; }

    Error _error = Error::Code::OK;

    SharedPtr<Executable> _executable;

#ifndef NDEBUG
    String _name;
#endif

private:
    CallReturnValue execute() { return _executable ? _executable->execute() : CallReturnValue(CallReturnValue::Type::Finished); }
    
    void finish() { if (_finishCB) _finishCB(this); }

    TaskManager::FinishCallback _finishCB;
        
    State _state = State::Ready;
};

}
