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
#include "MLittleFS.h"
#include "RtosWifi.h"
#include "SystemInterface.h"
#include "esp_system.h"
#include "spi_flash.h"
#include "freertos/FreeRTOS.h"

using namespace m8r;

static constexpr uint32_t FSStart = 0x100000;

int32_t m8r::heapFreeSize()
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

void IPAddr::lookupHostName(const char* name, std::function<void (const char* name, IPAddr)> func)
{
}

class RtosSystemInterface : public SystemInterface
{
public:
    RtosSystemInterface()
    {
    }
    
    virtual void vprintf(ROMString fmt, va_list args) const override
    {
        m8r::String s = m8r::String::vformat(fmt, args);
        ::printf(s.c_str());
    }
    
    virtual void setDeviceName(const char* name) { }
    
    virtual FS* fileSystem() override { return &_fileSystem; }
    virtual GPIOInterface* gpio() override { return nullptr; }
    
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
    RtosWifi _wifi;
};

extern uint64_t g_esp_os_us;

SystemInterface* SystemInterface::create() { return new RtosSystemInterface(); }

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
