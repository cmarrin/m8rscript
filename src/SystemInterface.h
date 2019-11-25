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
class TCP;
class TCPDelegate;
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
    virtual TaskManager* taskManager() = 0;
    virtual Mad<TCP> createTCP(TCPDelegate* delegate, uint16_t port, IPAddr ip = IPAddr()) = 0;
    virtual Mad<UDP> createUDP(UDPDelegate* delegate, uint16_t port) = 0;
    
    static void heapInfo(void*& start, uint32_t& size);
    
    virtual void setDeviceName(const char*) = 0;
    
    virtual void vprintf(ROMString, va_list) const = 0;

    static void gcLock();
    static void gcUnlock();
    static void mallocatorLock();
    static void mallocatorUnlock();
    static void eventLock();
    static void eventUnlock();

    void runLoop();

private:
    static uint64_t currentMicroseconds();
};

static inline SystemInterface* system() { return SystemInterface::get(); }

}
