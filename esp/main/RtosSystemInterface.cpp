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
#include "LittleFS.h"
#include "RtosTaskManager.h"
#include "SystemInterface.h"
#include "esp_system.h"
#include "spi_flash.h"

using namespace m8r;

static constexpr uint32_t FSStart = 0x100000;

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

void m8r::heapInfo(void*& start, uint32_t& size)
{
    static void* heapAddr = nullptr;
    static uint32_t heapSize = 0;
    
    if (!heapAddr) {
        heapSize = heap_caps_get_free_size(MALLOC_CAP_8BIT) - 10000;
        heapAddr = heap_caps_malloc(heapSize, MALLOC_CAP_8BIT);
    }
    
    start = heapAddr;
    size = heapSize;

    printf("\n*** heap start=%p, size=%d\n", start, size);
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
    
    virtual FS* fileSystem() override { return &_fileSystem; }
    virtual GPIOInterface* gpio() override { return nullptr; }
    virtual TaskManager* taskManager() override { return &_taskManager; };
    
    virtual Mad<TCP> createTCP(uint16_t port, m8r::IPAddr ip, TCP::EventFunction) override
    {
        return Mad<TCP>();
    }
    
    virtual Mad<TCP> createTCP(uint16_t port, TCP::EventFunction) override
    {
        return Mad<TCP>();
    }
    
    virtual Mad<UDP> createUDP(uint16_t port, UDP::EventFunction) override
    {
        return Mad<UDP>();
    }

private:
//    RtosGPIOInterface _gpio;
    LittleFS _fileSystem;
    RtosTaskManager _taskManager;
};

extern uint64_t g_esp_os_us;

static RtosSystemInterface _gSystemInterface;

m8r::SystemInterface* m8r::SystemInterface::get() { return &_gSystemInterface; }

static int lfs_flash_read(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, void *dst, lfs_size_t size)
{
    uint32_t addr = (block * LittleFS::BlockSize) + off + FSStart;
    return spi_flash_read(addr, static_cast<uint8_t*>(dst), size) == 0 ? 0 : -1;
}

static int lfs_flash_write(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t addr = (block * LittleFS::BlockSize) + off + FSStart;
    return (spi_flash_write(addr, buffer, size) == 0) ? 0 : -1;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    return (spi_flash_erase_sector(block + (FSStart / LittleFS::BlockSize)) == 0) ? 0 : -1;
}

static int lfs_flash_sync(const struct lfs_config *c) {
    /* NOOP */
    (void) c;
    return 0;
}

void LittleFS::setConfig(lfs_config& config)
{
    config.read = lfs_flash_read;
    config.prog = lfs_flash_write;
    config.erase = lfs_flash_erase;
    config.sync = lfs_flash_sync;
}
