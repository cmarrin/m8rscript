/*
 Esp.cpp - ESP8266-specific APIs
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Defines.h"
//#include "RtosSpiffsFS.h"
#include "RtosTaskManager.h"
#include "SystemInterface.h"
#include "esp_system.h"

using namespace m8r;

void* ROMmemcpy(void* dst, m8r::ROMString src, size_t len)
{
    uint8_t* d = (uint8_t*) dst;
    while (len--) {
        *d++ = readRomByte(src);
        src += 1;
    }
    return dst;
}

char* ROMCopyString(char* dst, m8r::ROMString src)
{
    char c;
    while ((c = (char) readRomByte(src))) {
        *dst++ = c;
        src += 1;
    }
    *dst = '\0';
    return dst;
}

size_t ROMstrlen(m8r::ROMString s)
{
    m8r::ROMString p = s;
    for ( ; readRomByte(p) != '\0'; p += 1) ;
    return (size_t) (p.value() - s.value());
}

m8r::ROMString ROMstrstr(m8r::ROMString s1, const char* s2)
{
    int i, j;

    if (!s1.valid() || s2 == nullptr) {
        return m8r::ROMString();
    }

    for( i = 0; ; i++) {
        char c1 = readRomByte(s1 + i);
        if (c1 == '\0') {
            return m8r::ROMString();
        }
        
        char c2 = *s2;
        if (c1 == c2) {
            for (j = i; ; j++) {
                c2 = *(s2 + (j - i));
                if (c2 == '\0') {
                    return m8r::ROMString(s1 + i);
                }
                c1 = readRomByte(s1 + j);
                if (c1 != c2) {
                    break;
                }
            }
        }
    }
}

int ROMstrcmp(m8r::ROMString s1, const char* s2)
{
    uint8_t c1;
    uint8_t c2;
    for (int32_t i = 0; ; i++) {
        c1 = readRomByte(s1+i);
        c2 = s2[i];
        if (c1 != c2) {
            break;
        }
        if (c1 == '\0') {
            return 0;
        }
    }
    return c1 - c2;
}

void IPAddr::lookupHostName(const char* name, std::function<void (const char* name, IPAddr)> func)
{
}

class RtosSystemInterface : public SystemInterface
{
public:
    RtosSystemInterface()
    {
        srand(esp_random());
    }
    
    virtual void vprintf(ROMString fmt, va_list args) const override
    {
        m8r::String s = m8r::String::vformat(fmt, args);
        ::printf(s.c_str());
    }
    
    virtual void setDeviceName(const char* name) { }
    
    virtual FS* fileSystem() override { return nullptr; /*&_fileSystem;*/ }
    virtual GPIOInterface* gpio() override { return nullptr; }
    virtual TaskManager* taskManager() override { return &_taskManager; };
    
    virtual Mad<TCP> createTCP(m8r::TCPDelegate* delegate, uint16_t port, m8r::IPAddr ip = m8r::IPAddr()) override
    {
        return Mad<TCP>();
    }
    
    virtual Mad<UDP> createUDP(UDPDelegate* delegate, uint16_t port) override
    {
        return Mad<UDP>();
    }

private:
//    RtosGPIOInterface _gpio;
//    SpiffsFS _fileSystem;
    RtosTaskManager _taskManager;
};

extern uint64_t g_esp_os_us;

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    return g_esp_os_us;
}

static RtosSystemInterface _gSystemInterface;

m8r::SystemInterface* m8r::SystemInterface::get() { return &_gSystemInterface; }
