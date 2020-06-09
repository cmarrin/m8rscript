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
    
    virtual int8_t startTimer(Duration duration, bool repeat, std::function<void()> cb) override
    {
        int8_t id = -1;
        
        {
            Lock lock(_timerMutex);
            for (int i = 0; i < NumTimers; ++i) {
                if (!_timers[i].allocated) {
                    id = i;
                    break;
                }
            }
            
            if (id < 0) {
                return id;
            }
        
            _timers[id].running = true;
            _timers[id].allocated = true;
        }
        
        Thread(1024, [this, id, duration, repeat, cb] {
            while (1) {
                {
                    Lock lock(_timers[id].mutex);
                    if (!_timers[id].running) {
                        // We've been stopped
                        break;
                    }
                    
                    if (_timers[id].cond.waitFor(lock, std::chrono::microseconds(duration.us())) != Condition::WaitResult::TimedOut) {
                        // Timer stopped
                        _timers[id].running = false;
                        break;
                    }
                    
                    cb();
                    if (repeat) {
                        continue;
                    } else {
                        _timers[id].running = false;
                        break;
                    }
                }
            }
            _timers[id].allocated = false;
        }).detach();
        
        return id;
    }

    virtual void stopTimer(int8_t id) override
    {
        {
            Lock lock(_timerMutex);

            if (id < 0 || id >= NumTimers || !_timers[id].running) {
                return;
            }
        }
        
        Lock lock(_timers[id].mutex);
        _timers[id].cond.notify(true);
        _timers[id].running = false;
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

    struct TimerEntry
    {
        bool running = false;
        bool allocated = false;
        Condition cond;
        Mutex mutex;
    };
    TimerEntry _timers[NumTimers];
    Mutex _timerMutex;
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
