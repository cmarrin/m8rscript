/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MacSystemInterface.h"

#include "GPIOInterface.h"
#include "TaskManager.h"
#include "Thread.h"
#include "MacTCP.h"
#include "MacUDP.h"
#include "SystemInterface.h"

#include "MLittleFS.h"
#include "cpptime.h"

using namespace m8r;

static ConsoleCB _consoleCB;

class MacSystemInterface : public SystemInterface
{
public:
    static constexpr int NumTimers = 8;

    MacSystemInterface()
    {
    }
    
    virtual void print(const char* s) const override
    {
        if (_consoleCB) {
            _consoleCB(s);
        } else {
            ::printf("%s", s);
        }
    }
    
    virtual void setDeviceName(const char*) override { }
    virtual FS* fileSystem() override { return &_fileSystem; }
    virtual GPIOInterface* gpio() override { return &_gpio; }
    
    virtual Mad<TCP> createTCP(uint16_t port, IPAddr ip, TCP::EventFunction func) override
    {
        Mad<MacTCP> tcp = Mad<MacTCP>::create(MemoryType::Network);
        tcp->init(port, ip, func);
        return tcp;
    }
    
    virtual Mad<TCP> createTCP(uint16_t port, TCP::EventFunction func) override
    {
        Mad<MacTCP> tcp = Mad<MacTCP>::create(MemoryType::Network);
        tcp->init(port, IPAddr(), func);
        return tcp;
    }
    
    virtual Mad<UDP> createUDP(uint16_t port, UDP::EventFunction func) override
    {
        Mad<MacUDP> udp = Mad<MacUDP>::create(MemoryType::Network);
        udp->init(port, func);
        return udp;
    }

private:
    GPIOInterface _gpio;
    LittleFS _fileSystem;
};

int32_t SystemInterface::heapFreeSize()
{
    return -1;
}

SystemInterface* SystemInterface::create() { return new MacSystemInterface(); }

void m8r::initMacSystemInterface(const char* fsFile, ConsoleCB cb)
{
    _consoleCB = cb;
    LittleFS::setHostFilename(fsFile);
}

CppTime::Timer _timerManager;

Timer::~Timer()
{
    stop();
}

void Timer::init()
{
}

void Timer::start(Duration duration, Behavior behavior)
{
    stop();
    
    std::chrono::milliseconds dur = std::chrono::milliseconds(duration.ms());
    std::chrono::milliseconds repeat;
    if (behavior == Behavior::Repeating) {
        repeat = dur;
    }
    CppTime::timer_id id = _timerManager.add(dur, [this](CppTime::timer_id)
    {
        _cb(this);
    }, repeat);
    _data = reinterpret_cast<void*>(id);
}

void Timer::stop()
{
    if (_data) {
        CppTime::timer_id id = reinterpret_cast<CppTime::timer_id>(_data);
        _timerManager.remove(id);
        _data = nullptr;
    }
}

bool Timer::running() const
{
    return _data;
}
