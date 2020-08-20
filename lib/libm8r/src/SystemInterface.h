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
#include "IPAddr.h"
#include "ScriptingLanguage.h"
#include "SharedPtr.h"
#include "TaskManager.h"
#include "TCP.h"
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <memory>
#include <vector>

namespace m8r {

class GPIOInterface;
class FS;
class TaskManager;
class UDP;
class UDPDelegate;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SystemInterface
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class SystemInterface  {
public:
    static SystemInterface* create();

    virtual ~SystemInterface() { }
    
    virtual void init() { }

    virtual void print(const char* s) const = 0;
    
    void printf(const char* fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    }

    void vprintf(const char* fmt, va_list args) const
    {
        print(String::vformat(fmt, args).c_str());
    }
    
    virtual FS* fileSystem() = 0;
    virtual GPIOInterface* gpio() = 0;
    virtual Mad<TCP> createTCP(uint16_t port, IPAddr ip, TCP::EventFunction) = 0;
    virtual Mad<TCP> createTCP(uint16_t port, TCP::EventFunction) = 0;
    virtual Mad<UDP> createUDP(uint16_t port, UDP::EventFunction) = 0;
    
    TaskManager* taskManager() { return &_taskManager; };

    virtual void setDeviceName(const char*) = 0;
        
    void receivedLine(const char* line)
    {
        if(_listenerFunc) {
            _listenerFunc(line);
        }
    }
    
    void setListenerFunc(std::function<void(const char*)> func) { _listenerFunc = func; }

    bool runOneIteration();
    
    void setHeartrate(Duration rate, Duration ontime = Duration());
    void setDefaultHeartOnTime(Duration ontime);
    
    static int32_t heapFreeSize();
    
    void registerScriptingLanguage(const ScriptingLanguage*);
    const ScriptingLanguage* scriptingLanguage(uint32_t i)
    {
        if (i >= _scriptingLanguages.size()) {
            return nullptr;
        }
        return _scriptingLanguages[i];
    }

protected:
    SystemInterface();

private:
    void startHeartbeat();
    
    std::function<void(const char*)> _listenerFunc;
    TaskManager _taskManager;
    
    Timer _heartbeatTimer;
    Duration _heartrate;
    Duration _heartOnTime;
    Duration _defaultHeartOnTime;
    bool _heartOn = false;
    
    Vector<const ScriptingLanguage*> _scriptingLanguages;
};

SystemInterface* system();

}
