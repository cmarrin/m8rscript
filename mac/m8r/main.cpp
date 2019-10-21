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
#include <stdio.h>

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
    
    virtual int getChar() const override
    {
        return getchar();
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
                "usage: %s [-p <port>] [-u <file>] [-l <path>] [-h] <dir>\n"
                "    -p   : set shell port (log port +1, sim port +2)\n"
                "    -u   : upload file (use file's basename as Spiffs name)\n"
                "    -l   : path for uploaded file (default '/')\n"
                "    -h   : print this message\n"
                "    <dir>: root directory for simulation filesystem\n"
            , name);
}

int main(int argc, char * argv[])
{
    int opt;
    uint16_t port = 23;
    const char* uploadFilename = nullptr;
    const char* uploadPath = "/";
    
    while ((opt = getopt(argc, argv, "p:u:l:h")) != EOF) {
        switch(opt)
        {
            case 'p': port = atoi(optarg); break;
            case 'u': uploadFilename = optarg; break;
            case 'l': uploadPath = optarg; break;
            case 'h': usage(argv[0]); break;
            default : break;
        }
    }
    
    // Seed the random number generator
    srand(static_cast<uint32_t>(time(nullptr)));
    
    const char* fsdir = (optind < argc) ? argv[optind] : "SpiffsFSFile";
    _gSystemInterface =  std::unique_ptr<m8r::SystemInterface>(new MySystemInterface(fsdir));
    
    m8r::Application::mountFileSystem();
    
    // Upload the file if present
    if (uploadFilename) {
        m8r::String toPath;
        FILE* fromFile = fopen(uploadFilename, "r");
        if (!fromFile) {
            fprintf(stderr, "Unable to open '%s' for upload, skipping\n", uploadFilename);
        } else {
            std::vector<m8r::String> parts = m8r::String(uploadFilename).split("/");
            m8r::String baseName = parts[parts.size() - 1];
            
            if (uploadPath[0] != '/') {
                toPath += '/';
            }
            toPath += uploadPath;
            if (toPath[toPath.size() - 1] != '/') {
                toPath += '/';
            }
            
            // Make sure the directory path exists
            m8r::system()->fileSystem()->makeDirectory(toPath.c_str());
            if (m8r::system()->fileSystem()->lastError() != m8r::Error::Code::OK) {
                printf("Error: unable to create '%s' on Spiffs file system - ", toPath.c_str());
                m8r::Error::showError(m8r::system()->fileSystem()->lastError());
                printf("\n");
            } else {
                toPath += baseName;
                
                std::shared_ptr<m8r::File> toFile = m8r::system()->fileSystem()->open(toPath.c_str(), m8r::FS::FileOpenMode::Write);
                if (!toFile->valid()) {
                    printf("Error: unable to open '%s' on Spiffs file system - ", toPath.c_str());
                    m8r::Error::showError(toFile->error());
                    printf("\n");
                } else {
                    bool success = true;
                    while(1) {
                        char c;
                        size_t size = fread(&c, 1, 1, fromFile);
                        if (size != 1) {
                            if (!feof(fromFile)) {
                                fprintf(stderr, "Error reading '%s', upload failed\n", uploadFilename);
                                success = false;
                            }
                            break;
                        }
                        
                        toFile->write(c);
                        if (!toFile->valid()) {
                            fprintf(stderr, "Error writing '%s', upload failed\n", toPath.c_str());
                            success = false;
                            break;
                        }
                    }
                    if (success) {
                        printf("Uploaded '%s' to '%s'\n", uploadFilename, toPath.c_str());
                    }
                }
            }
        }
    }

    m8r::Application application(port);
    m8r::Application::mountFileSystem();
    application.runLoop();

    return 0;
}
