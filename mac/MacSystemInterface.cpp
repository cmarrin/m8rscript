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

#include "MLittleFS.h"

using namespace m8r;

static ConsoleCB _consoleCB;

class MacSystemInterface : public SystemInterface
{
public:
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

int32_t m8r::heapFreeSize()
{
    return -1;
}

SystemInterface* SystemInterface::create() { return new MacSystemInterface(); }

void m8r::initMacSystemInterface(const char* fsFile, ConsoleCB cb)
{
    _consoleCB = cb;
    LittleFS::setHostFilename(fsFile);
    
}
