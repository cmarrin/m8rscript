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

struct ErrorEntry {
    ErrorEntry(const char* description, uint32_t lineno, uint16_t charno = 1, uint16_t length = 1)
        : _lineno(lineno)
        , _charno(charno)
        , _length(length)
    {
        size_t size = strlen(description);
        _description = new char[size + 1];
        memcpy(_description, description, size + 1);
    }
    
    ErrorEntry(const ErrorEntry& other)
        : _lineno(other._lineno)
        , _charno(other._charno)
        , _length(other._length)
    {
        size_t size = strlen(other._description);
        _description = new char[size + 1];
        memcpy(_description, other._description, size + 1);
    }
    
    ~ErrorEntry()
    {
        delete [ ] _description;
    }
    
    char* _description;
    uint32_t _lineno;
    uint16_t _charno;
    uint16_t _length;
};

typedef std::vector<ErrorEntry> ErrorList;

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

    void printf(const char* fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    }
    
    virtual FS* fileSystem() = 0;
    virtual GPIOInterface* gpio() = 0;
    virtual TaskManager* taskManager() = 0;
    virtual std::unique_ptr<TCP> createTCP(TCPDelegate* delegate, uint16_t port, IPAddr ip = IPAddr()) = 0;
    virtual std::unique_ptr<UDP> createUDP(UDPDelegate* delegate, uint16_t port) = 0;
    
    static void* alloc(MemoryType, size_t);
    static void free(MemoryType, void*);
    
    virtual void setDeviceName(const char*) = 0;
    
    virtual void vprintf(const char*, va_list) const = 0;
    
    static void memoryInfo(MemoryInfo&);
    
    void lock();
    void unlock();
    
    void runLoop();

private:
    static uint64_t currentMicroseconds();
};

static inline SystemInterface* system() { return SystemInterface::get(); }

}
