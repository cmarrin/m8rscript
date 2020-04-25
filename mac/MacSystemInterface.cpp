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
#include "MacTCP.h"
#include "MacUDP.h"
#include "SystemInterface.h"
#include <condition_variable>
#include <mutex>
#include <thread>

#ifndef USE_LITTLEFS
#include "SpiffsFS.h"
#else
#include "MLittleFS.h"
#endif

using namespace m8r;

static constexpr int NumTimers = 8;

class MacSystemInterface : public SystemInterface
{
public:
    MacSystemInterface()
    {
    }
    
    virtual void vprintf(ROMString s, va_list args) const override
    {
        String ss(s);
        ::vprintf(ss.c_str(), args);
    }
    
    virtual int8_t startTimer(Duration duration, bool repeat, std::function<void()> cb) override
    {
        int8_t id = -1;
        
        for (int i = 0; i < NumTimers; ++i) {
            if (!_timers[i].running) {
                id = i;
                break;
            }
        }
        
        if (id < 0) {
            return id;
        }
        
        _timers[id]. running = true;
        
        std::thread([this, id, duration, repeat, cb] {
            while (1) {
                {
                    std::unique_lock<std::mutex> lock(_timers[id].mutex);
                    if (_timers[id].cond.wait_for(lock, std::chrono::microseconds(duration.us())) != std::cv_status::timeout) {
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
        }).detach();
        
        return id;
    }
    
    virtual void stopTimer(int8_t id) override
    {
        if (id < 0 || id >= NumTimers || !_timers[id].running) {
            return;
        }
        
        std::unique_lock<std::mutex> lock(_timers[id].mutex);
        _timers[id].cond.notify_all();
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
    class MyGPIOInterface : public GPIOInterface {
    public:
        MyGPIOInterface() { }
        virtual ~MyGPIOInterface() { }

        virtual bool setPinMode(uint8_t pin, PinMode mode) override
        {
            ::printf("*** setPinMode(%d, %d)\n", pin, static_cast<uint32_t>(mode));
            _pinio = (_pinio & ~(1 << pin)) | ((mode == PinMode::Output) ? (1 << pin) : 0);
            return true;
        }
        
        virtual bool digitalRead(uint8_t pin) const override
        {
            ::printf("*** digitalRead(%d) ==> %s\n", pin, (_pinstate & (1 << pin)) ? "true" : "false");
            return _pinstate & (1 << pin);
        }
        
        virtual void digitalWrite(uint8_t pin, bool level) override
        {
            if (pin > 16) {
                return;
            }
            _pinstate = (_pinstate & ~(1 << pin)) | (level ? (1 << pin) : 0);
            ::printf("*** digitalWrite(%d, %s)\n", pin, level ? "true" : "false");
        }
        
        virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) override { }
        
    private:
        // 0 = input, 1 = output
        uint32_t _pinio = 0;
        uint32_t _pinstate = 0;
    };
    
    MyGPIOInterface _gpio;
#ifndef USE_LITTLEFS
    SpiffsFS _fileSystem;
#else
    LittleFS _fileSystem;
#endif

    struct TimerEntry
    {
        bool running = false;
        std::condition_variable cond;
        std::mutex mutex;
    };
    TimerEntry _timers[NumTimers];
};

int32_t m8r::heapFreeSize()
{
    return -1;
}

SystemInterface* SystemInterface::create() { return new MacSystemInterface(); }

#ifdef USE_LITTLEFS
void m8r::initMacFileSystem(const char* fsFile) { LittleFS::setHostFilename(fsFile); }
#else
void m8r::initMacFileSystem(const char* fsFile) { SpiffsFS::setHostFilename(fsFile); }
#endif
