/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "CallReturnValue.h"
#include "Containers.h"
#include "Error.h"
#include "MStream.h"
#include "SystemInterface.h"
#include "TaskManager.h"
#include <cstdint>
#include <functional>

namespace m8r {

class String;

class Task : public NativeObject {
    friend class TaskManager;
    
public:
    class Executable
    {
    public:
        using ConsoleListener = std::function<void(const String& data, KeyAction)>;
        
        Executable() { }
        virtual ~Executable() { }
        
        virtual CallReturnValue execute() = 0;
        virtual bool readyToRun() const { return true; }
        virtual void requestYield() const { }
        virtual void receivedData(const String& data, KeyAction) { }
        
        void printf(ROMString fmt, ...) const
        {
            va_list args;
            va_start(args, fmt);
            vprintf(fmt, args);
        }

        void vprintf(ROMString fmt, va_list args) const
        {
            print(ROMString::vformat(fmt, args).c_str());
        }
        
        void print(const char* s) const;

        void setConsolePrintFunction(const std::function<void(const String&)>& f) { _consolePrintFunction = std::move(f); }
        std::function<void(const String&)> consolePrintFunction() const { return _consolePrintFunction; }
 
    private:
        std::function<void(const String&)> _consolePrintFunction;
    };
        
    enum class State { Ready, WaitingForEvent, Delaying, Terminated };
    
    Task() { }
    
    ~Task();
    
    bool run(const std::shared_ptr<Executable>& exec)
    {
        _executable = exec;
        return true;
    }

#if M8RSCRIPT_SUPPORT == 1 || MARLY_SUPPORT == 1
    bool run(const Stream&);
    bool run(const char* filename);
#endif

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

#ifndef NDEBUG
    const String& name() const { return _name; }
#endif

protected:
    State state() const { return _state; }
    void setState(State state) { _state = state; }

    Error _error = Error::Code::OK;

    std::shared_ptr<Executable> _executable;

#ifndef NDEBUG
    String _name;
#endif

private:
    CallReturnValue execute() { return _executable ? _executable->execute() : CallReturnValue(CallReturnValue::Type::Finished); }
    
    void finish() { if (_finishCB) _finishCB(this); }

    TaskManager::FinishCallback _finishCB;
        
    State _state = State::Ready;
};

#if M8RSCRIPT_SUPPORT == 1
class TaskProto : public StaticObject {
public:
    TaskProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue run(ExecutionUnit*, Value thisValue, uint32_t nparams);
};
#endif

}
