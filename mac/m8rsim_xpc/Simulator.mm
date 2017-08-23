//
//  Simulator.mm
//  m8rscript
//
//  Created by Chris Marrin on 7/6/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#include "Simulator.h"

#import "m8rsim_xpcProtocol.h"
#import "Application.h"
#import "GPIOInterface.h"
#import "IPAddr.h"
#import "MacFS.h"
#import "SystemInterface.h"
#import <chrono>
#import <malloc/malloc.h>

static malloc_zone_t* g_defaultZone;
static void* (*g_defaultMalloc)(malloc_zone_t*, size_t);
static void (*g_defaultFree)(malloc_zone_t*, void*);

static uint32_t g_allocCount;
static uint32_t g_freeHeapSize;

static void* g_malloc(malloc_zone_t* zone, size_t size)
{
    ++g_allocCount;
    g_freeHeapSize -= size;
    return g_defaultZone->malloc(zone, size);
}

static void g_free(malloc_zone_t* zone, void* ptr)
{
    --g_allocCount;
    g_freeHeapSize += malloc_size(ptr);
    g_defaultZone->free(zone, ptr);
}

class MyLogSocket : public m8r::TCPDelegate {
public:
    MyLogSocket(uint16_t port)
        : _tcp(m8r::TCP::create(this, port))
    { }
    
    virtual ~MyLogSocket() { delete _tcp; }

    virtual void TCPevent(m8r::TCP* tcp, m8r::TCPDelegate::Event event, int16_t connectionId, const char* data, int16_t length) override
    {
        if (event == m8r::TCPDelegate::Event::Connected) {
            tcp->send(connectionId, "Start m8rscript Log\n\n");
        }
    }

    void log(const char* s)
    {
        for (uint16_t connection = 0; connection < m8r::TCP::MaxConnections; ++connection) {
            _tcp->send(connection, s);
        }
    }
private:
    m8r::TCP* _tcp;
};

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    return static_cast<uint64_t>(std::clock() * 1000000 / CLOCKS_PER_SEC);
}

malloc_zone_t* g_m8rzone = nullptr;

void* m8r::SystemInterface::alloc(MemoryType type, size_t size)
{
    return ::malloc(size);
}

void m8r::SystemInterface::free(MemoryType, void* p)
{
    ::free(p);
}

void m8r::SystemInterface::memoryInfo(MemoryInfo& info)
{
    info.freeSize = g_freeHeapSize;
    info.numAllocations = g_allocCount;
}

class MySystemInterface : public m8r::SystemInterface
{
public:
    MySystemInterface(uint16_t port)
    {
        _logSocket = new MyLogSocket(port + 1);
    }
    
    virtual ~MySystemInterface()
    {
        delete _logSocket;
    }
    
    virtual void vprintf(const char* s, va_list args) const override
    {
        NSString* string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:s] arguments:args];
        _logSocket->log([string UTF8String]);
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
            if (!GPIOInterface::setPinMode(pin, mode)) {
                return false;
            }
            _pinio = (_pinio & ~(1 << pin)) | ((mode == PinMode::Output) ? (1 << pin) : 0);

            // FIXME: Implement
            //[_device updateGPIOState:_pinio withMode:_pinstate];
            return true;
        }
        
        virtual bool digitalRead(uint8_t pin) const override
        {
            return _pinstate & (1 << pin);
        }
        
        virtual void digitalWrite(uint8_t pin, bool level) override
        {
            if (pin > 16) {
                return;
            }
            _pinstate = (_pinstate & ~(1 << pin)) | (level ? (1 << pin) : 0);

            // FIXME: Implement
            //[_device updateGPIOState:_pinstate withMode:_pinio];
        }
        
        virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) override { }
        
    private:
        // 0 = input, 1 = output
        uint32_t _pinio = 0;
        uint32_t _pinstate = 0;
    };
    
    MyGPIOInterface _gpio;
    MyLogSocket* _logSocket;
};

@interface Simulator ()
{
    NSXPCConnection* _xpc;
    m8r::FS* _fs;
    m8r::Application* _application;
    MySystemInterface* _system;
    
    NSFileWrapper* _files;
    NSInteger _status;
}

@end

@implementation Simulator

@synthesize status = _status;

- (instancetype)initWithPort:(NSUInteger)port
{
    self = [super init];
    if (self) {
        (void) m8r::IPAddr::myIPAddr();
        
        g_allocCount = 0;
        g_freeHeapSize = 88000;
        g_defaultZone = malloc_default_zone();
        g_defaultMalloc = g_malloc;
        g_defaultFree = g_free;
        
        _system = new MySystemInterface(port);
        _fs = m8r::FS::createFS();
        _application = new m8r::Application(_fs, _system, port);
        
        _status = 0;
    }
    return self;
}

- (void)dealloc
{
     [_xpc invalidate];
     delete _application;
     delete _fs;
     delete _system;
}


- (NSInteger)setFiles:(NSURL*)files
{
    NSFileWrapper* wrapper = [[NSFileWrapper alloc] initWithURL:files options:0 error:NULL];
    static_cast<m8r::MacFS*>(_fs)->setFiles(wrapper);
    return 0;
}

@end
