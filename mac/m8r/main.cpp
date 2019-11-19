/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>

#include "Application.h"
#include "MacTaskManager.h"
#include "MacTCP.h"
#include "MacUDP.h"
#include "SystemInterface.h"

#ifndef USE_LITTLEFS
#include "SpiffsFS.h"
#else
#include "LittleFS.h"
#endif

class MySystemInterface : public m8r::SystemInterface
{
public:
    void init(const char* fsFile) { _fileSystem.init(fsFile); }
    
    virtual void vprintf(m8r::ROMString s, va_list args) const override
    {
        m8r::String ss(s);
        ::vprintf(ss.c_str(), args);
    }
    
    virtual void setDeviceName(const char*) override { }
    virtual m8r::FS* fileSystem() override { return &_fileSystem; }
    virtual m8r::GPIOInterface* gpio() override { return &_gpio; }
    virtual m8r::TaskManager* taskManager() override { return &_taskManager; };
    
    virtual m8r::Mad<m8r::TCP> createTCP(m8r::TCPDelegate* delegate, uint16_t port, m8r::IPAddr ip = m8r::IPAddr()) override
    {
        m8r::Mad<m8r::MacTCP> tcp = m8r::Mad<m8r::MacTCP>::create(m8r::MemoryType::Network);
        tcp->init(delegate, port, ip);
        return tcp;
    }
    
    virtual m8r::Mad<m8r::UDP> createUDP(m8r::UDPDelegate* delegate, uint16_t port) override
    {
        m8r::Mad<m8r::MacUDP> udp = m8r::Mad<m8r::MacUDP>::create(m8r::MemoryType::Network);
        udp->init(delegate, port);
        return udp;
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
#ifndef USE_LITTLEFS
    m8r::SpiffsFS _fileSystem;
#else
    m8r::LittleFS _fileSystem;
#endif
    m8r::MacTaskManager _taskManager;
};

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    return static_cast<uint64_t>(std::clock() * 1000000 / CLOCKS_PER_SEC);
}

void m8r::SystemInterface::heapInfo(void*& start, uint32_t& size)
{
    static void* heap = nullptr;
    if (!heap) {
        heap = ::malloc(HeapSize);
    }
    start = heap;
    size = HeapSize;
}

static MySystemInterface _gSystemInterface;

m8r::SystemInterface* m8r::SystemInterface::get() { return &_gSystemInterface; }

static void usage(const char* name)
{
    fprintf(stderr,
                "usage: %s [-p <port>] [-f <filesystem file>] [-l <path>] [-h] <dir> <file> ...\n"
                "    -p     : set shell port (log port +1, sim port +2)\n"
                "    -f     : simulated filesystem path\n"
                "    -l     : path for uploaded files (default '/')\n"
                "    -h     : print this message\n"
                "    <file> : file(s) to be uploaded\n"
            , name);
}

int main(int argc, char * argv[])
{
    int opt;
    uint16_t port = 23;
    const char* fsFile = "m8rFSFile";
    const char* uploadPath = "/";
    
    while ((opt = getopt(argc, argv, "p:u:l:h")) != EOF) {
        switch(opt)
        {
            case 'p': port = atoi(optarg); break;
            case 'f': fsFile = optarg; break;
            case 'l': uploadPath = optarg; break;
            case 'h': usage(argv[0]); break;
            default : break;
        }
    }
    
    // Seed the random number generator
    srand(static_cast<uint32_t>(time(nullptr)));
    
    // TODO: The file system gets corrupted often. Let's recreate it every time until it is fixed
    remove(fsFile);
    
    _gSystemInterface.init(fsFile);
    
    m8r::Application::mountFileSystem();
    
    // Upload files if present
    for (int i = optind; i < argc; ++i) {
        const char* uploadFilename = argv[i];

        m8r::String toPath;
        FILE* fromFile = fopen(uploadFilename, "r");
        if (!fromFile) {
            fprintf(stderr, "Unable to open '%s' for upload, skipping\n", uploadFilename);
        } else {
            m8r::Vector<m8r::String> parts = m8r::String(uploadFilename).split("/");
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
                printf("Error: unable to create '%s' - ", toPath.c_str());
                m8r::Error::showError(m8r::system()->fileSystem()->lastError());
                printf("\n");
            } else {
                toPath += baseName;
                
                m8r::Mad<m8r::File> toFile(m8r::system()->fileSystem()->open(toPath.c_str(), m8r::FS::FileOpenMode::Write));
                if (!toFile->valid()) {
                    printf("Error: unable to open '%s' - ", toPath.c_str());
                    m8r::Error::showError(toFile->error());
                    printf("\n");
                } else {
                    bool success = true;
                    while (1) {
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
                    toFile->close();
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
