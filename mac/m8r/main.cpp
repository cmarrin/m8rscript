//
//  main.cpp
//  m8r
//
//  Created by Chris Marrin on 5/10/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include "Application.h"
#include "SpiffsFS.h"
#include "MacTaskManager.h"
#include "MacTCP.h"
#include "MacUDP.h"
#include "SystemInterface.h"

class MySystemInterface : public m8r::SystemInterface
{
public:
    MySystemInterface(const char* fsFile) : _fileSystem(fsFile) { }
    
    virtual void vprintf(const char* s, va_list args) const override
    {
        ::vprintf(s, args);
    }
    
    virtual void setDeviceName(const char*) override { }
    virtual m8r::FS* fileSystem() override { return &_fileSystem; }
    virtual m8r::GPIOInterface* gpio() override { return &_gpio; }
    virtual m8r::TaskManager* taskManager() override { return &_taskManager; };
    
    virtual std::unique_ptr<m8r::TCP> createTCP(m8r::TCPDelegate* delegate, uint16_t port, m8r::IPAddr ip = m8r::IPAddr()) override
    {
        return std::unique_ptr<m8r::TCP>(new m8r::MacTCP(delegate, port, ip));
    }
    
    virtual std::unique_ptr<m8r::UDP> createUDP(m8r::UDPDelegate* delegate, uint16_t port) override
    {
        return std::unique_ptr<m8r::UDP>(new m8r::MacUDP(delegate, port));
    }

private:
    class MyGPIOInterface : public m8r::GPIOInterface {
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
    m8r::SpiffsFS _fileSystem;
    m8r::MacTaskManager _taskManager;
};

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    return static_cast<uint64_t>(std::clock() * 1000000 / CLOCKS_PER_SEC);
}

void* m8r::SystemInterface::alloc(MemoryType type, size_t size)
{
    return ::malloc(size);
}

void m8r::SystemInterface::free(MemoryType, void* p)
{
    ::free(p);
}

void m8r::SystemInterface::memoryInfo(m8r::MemoryInfo& info)
{
    // FIXME: info.freeSize = g_freeHeapSize;
    // FIXME: info.numAllocations = g_allocCount;
}

static std::unique_ptr<m8r::SystemInterface> _gSystemInterface;

m8r::SystemInterface* m8r::SystemInterface::get() { return _gSystemInterface.get(); }

static void usage(const char* name)
{
    fprintf(stderr,
                "usage: %s [-p <port>] [-h] <dir>\n"
                "    -p   : set shell port (log port +1, sim port +2)\n"
                "    -h   : print this message\n"
                "    <dir>: root directory for simulation filesystem\n"
            , name);
}

int main(int argc, char * argv[])
{
    int opt;
    uint16_t port;
    
    while ((opt = getopt(argc, argv, "p:h")) != EOF) {
        switch(opt)
        {
            case 'p':
                port = atoi(optarg);
                break;
            case 'h':
                usage(argv[0]);
                break;
            default:
                break;
        }
    }
    
    const char* fsdir = (optind < argc) ? argv[optind] : nullptr;
    _gSystemInterface =  std::unique_ptr<m8r::SystemInterface>(new MySystemInterface(fsdir));
    
    m8r::Application application(23);
    application.runLoop();

    return 0;
}
