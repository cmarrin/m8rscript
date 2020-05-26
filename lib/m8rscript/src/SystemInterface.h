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
#include "TaskManager.h"
#include "TCP.h"
#include "Thread.h"
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
    friend class Time;

    static SystemInterface* create();

    virtual ~SystemInterface() { }
    
    virtual void init() { }

    virtual void print(const char* s) const = 0;
    
    void printf(ROMString fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    }

    void vprintf(ROMString fmt, va_list) const;
    
    virtual FS* fileSystem() = 0;
    virtual GPIOInterface* gpio() = 0;
    virtual Mad<TCP> createTCP(uint16_t port, IPAddr ip, TCP::EventFunction) = 0;
    virtual Mad<TCP> createTCP(uint16_t port, TCP::EventFunction) = 0;
    virtual Mad<UDP> createUDP(uint16_t port, UDP::EventFunction) = 0;
    
    TaskManager* taskManager() { return &_taskManager; };

    virtual void setDeviceName(const char*) = 0;
        
    // Return timer ID, -1 if can't start a timer
    virtual int8_t startTimer(Duration, bool repeat, std::function<void()>) { return -1; }
    virtual void stopTimer(int8_t id) { }
    
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

protected:
    SystemInterface() { }

private:
    void startHeartbeat();
    
    std::function<void(const char*)> _listenerFunc;
    TaskManager _taskManager;
    
    std::shared_ptr<Timer> _heartbeatTimer;
    Duration _heartrate;
    Duration _heartOnTime;
    Duration _defaultHeartOnTime;
    bool _heartOn = false;
};

SystemInterface* system();

}
