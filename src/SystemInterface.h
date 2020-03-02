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
    
    static SystemInterface* get();

	SystemInterface() { }
    virtual ~SystemInterface() { }

    void printf(ROMString fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    }
    
    virtual FS* fileSystem() = 0;
    virtual GPIOInterface* gpio() = 0;
    virtual Mad<TCP> createTCP(uint16_t port, IPAddr ip, TCP::EventFunction) = 0;
    virtual Mad<TCP> createTCP(uint16_t port, TCP::EventFunction) = 0;
    virtual Mad<UDP> createUDP(uint16_t port, UDP::EventFunction) = 0;

    virtual void setDeviceName(const char*) = 0;
    
    virtual void vprintf(ROMString, va_list) const = 0;
    
    void receivedLine(const char* line)
    {
        if(_listenerFunc) {
            _listenerFunc(line);
        }
    }
    
    void setListenerFunc(std::function<void(const char*)> func) { _listenerFunc = func; }

    void runLoop();

private:
    std::function<void(const char*)> _listenerFunc;
};

static inline SystemInterface* system() { return SystemInterface::get(); }

}
