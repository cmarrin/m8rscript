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
#include "MacNativeFS.h"
#include "SystemInterface.h"

class MySystemInterface : public m8r::SystemInterface
{
public:
    MySystemInterface() { }
    
    virtual void vprintf(const char* s, va_list args) const override
    {
        ::vprintf(s, args);
    }
    
    virtual void setDeviceName(const char*) override { }
    virtual m8r::GPIOInterface& gpio() override { return _gpio; }

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
    
    const char* fsdir = argv[optind];
    m8r::FS* fs = new m8r::MacNativeFS(fsdir);

    MySystemInterface system;
    m8r::Application application(fs, &system, 22);
    m8r::Error error;
    bool done = false;

//    if (application.load(error, true, inputFile)) {
//        auto start = std::chrono::system_clock::now();
//        application.run([start, &done]{
//            auto end = std::chrono::system_clock::now();
//            std::chrono::duration<double> diff = end - start;
//            printf(ROMSTR("\n\n*** Finished (run time:%fms)\n"), diff.count() * 1000);
//            done = true;
//        });
//    } else {
//        error.showError(&system);
//    }
    
    return 0;
}
