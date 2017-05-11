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
    
    virtual m8r::GPIOInterface& gpio() override { return _gpio; }
    virtual uint32_t freeMemory() const override { return 80000; }

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

int main(int argc, char * argv[])
{
    bool printCode = false;
    int opt;
    
    while ((opt = getopt(argc, argv, "ph?")) != EOF) {
        switch(opt)
        {
            case 'c':
                printCode = true;
                break;
            case 'h':
            case '?':
                fprintf(stderr, "usage:\n -?,\t-h: print this message\n\t-c: print generated code\n");
            default:
                break;
        }
    }
    
    const char* inputFile = argv[optind];

    m8r::FS* fs = new m8r::MacNativeFS();
    MySystemInterface system;
    m8r::Application application(fs, &system);
    m8r::Error error;
    bool done = false;
    if (application.load(error, true, inputFile)) {
        auto start = std::chrono::system_clock::now();
        application.run([start, &done]{
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = end - start;
            printf(ROMSTR("\n\n*** Finished (run time:%fms)\n"), diff.count() * 1000);
            done = true;
        });
    }
    
    while (!done) {
        sleep(1);
    }
    
    return 0;
}
