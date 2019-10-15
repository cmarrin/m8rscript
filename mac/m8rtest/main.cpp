//
//  main.cpp
//  m8rtest
//
//  Created by Chris Marrin on 10/13/2019.
//  Copyright © 2017-2019 MarrinTech. All rights reserved.
//

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sstream>

#include "Application.h"
#include "GPIOInterface.h"
#include "MacTaskManager.h"
#include "MacTCP.h"
#include "MacUDP.h"
#include "SpiffsFS.h"
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

static void testExpect(const char* s, const char* expected, const char* got, bool result)
{
    std::cout << "**** " << s << "\n";
    std::cout << "     expected '" << expected << "', got '" << got << "'\n";
    std::cout << "     test " << (result ? "passed" : "FAILED") << "\n";
}

template<typename T>
static void testExpect(const char* s, T expected, T got)
{
    std::stringstream expectedString;
    std::stringstream gotString;
    expectedString << expected;
    gotString << got;
    testExpect(s, expectedString.str().c_str(), gotString.str().c_str(), expected == got);
}

template<>
void testExpect<m8r::String>(const char* s, m8r::String expected, m8r::String got)
{
    testExpect(s, expected.c_str(), got.c_str(), expected == got);
}

template<>
void testExpect<m8r::Error::Code>(const char* s, m8r::Error::Code expected, m8r::Error::Code got)
{
    std::cout << "**** " << s << "\n";
    std::cout << "     expected '";
    m8r::Error::showError(expected);
    std::cout << "', got '";
    m8r::Error::showError(got);
    std::cout << "'\n     test " << ((expected == got) ? "passed" : "FAILED") << "\n\n";
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
    
    // Seed the random number generator
    srand(static_cast<uint32_t>(time(nullptr)));
    
    const char* fsdir = (optind < argc) ? argv[optind] : "SpiffsFSFile";
    _gSystemInterface =  std::unique_ptr<m8r::SystemInterface>(new MySystemInterface(fsdir));
    if (!m8r::Application::mountFileSystem()) {
        printf("**** Count not mount file system, exiting...\n");
        return -1;
    }

    // Run tests
    
    // Filesystem
    
    // Make sure file isn't there
    static const char* RootFileName = "/Foo";
    m8r::system()->fileSystem()->remove(RootFileName);
    
    // Open Read-only. Should fail
    std::shared_ptr<m8r::File> file = m8r::system()->fileSystem()->open(RootFileName, m8r::FS::FileOpenMode::Read);
    testExpect("Open non-existant file in Read mode error return", m8r::Error::Code::FileNotFound, file->error().code());

    file = m8r::system()->fileSystem()->open(RootFileName, m8r::FS::FileOpenMode::ReadUpdate);
    testExpect("Open non-existant file in ReadUpdate mode error return", m8r::Error::Code::FileNotFound, file->error().code());
    
    file = m8r::system()->fileSystem()->open(RootFileName, m8r::FS::FileOpenMode::WriteUpdate);
    testExpect("Open non-existant file in Write mode error return", m8r::Error::Code::OK, file->error().code());

    m8r::String testString = "The quick brown fox jumps over the lazy dog";
    file->write(testString.c_str(), static_cast<uint32_t>(testString.size()) + 1);
    testExpect("Write string to file error return", m8r::Error::Code::OK, file->error().code());
    
    file->seek(0);
    testExpect("Seek to 0 error return", m8r::Error::Code::OK, file->error().code());

    char buf[9];
    file->read(buf, 9);
    testExpect("Read error return", m8r::Error::Code::OK, file->error().code());
    testExpect("Read return value", m8r::String("The quick"), m8r::String(buf, 9));

    return 0;
}
