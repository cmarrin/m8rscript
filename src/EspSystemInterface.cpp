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
#include "littlefs/MLittleFS.h"
#include "MFS.h"
#include "SystemInterface.h"
#include "EspGPIOInterface.h"
#include "EspTCP.h"
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include "flash_hal.h"

using namespace m8r;

static constexpr const char* ConfigPortalName = "m8rscript";
static constexpr uint32_t FSStart = 0x100000;
static constexpr int NumTimers = 8;

void IPAddr::lookupHostName(const char* name, std::function<void (const char* name, IPAddr)> func)
{
}

m8r::ROMString ROMString::strstr(m8r::ROMString s1, const char* s2)
{
    int i, j;

    if (!s1.valid() || s2 == nullptr) {
        return m8r::ROMString();
    }

    for( i = 0; ; i++) {
        char c1 = ROMString::readByte(s1 + i);
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
                c1 = ROMString::readByte(s1 + j);
                if (c1 != c2) {
                    break;
                }
            }
        }
    }
}

class EspSystemInterface : public SystemInterface
{
public:    
    virtual void init() override
    {
        startNetwork();
    }

    virtual void print(const char* s) const override
    {
        ::printf("%s", s);
    }
    
    virtual void setDeviceName(const char* name) { }
    
    virtual m8r::FS* fileSystem() override { return &_fileSystem; }
    virtual GPIOInterface* gpio() override { return &_gpio; }
    
    virtual Mad<TCP> createTCP(uint16_t port, IPAddr ip, TCP::EventFunction func) override
    {
        Mad<EspTCP> tcp = Mad<EspTCP>::create(MemoryType::Network);
        tcp->init(port, ip, func);
        return tcp;
    }
    
    virtual Mad<TCP> createTCP(uint16_t port, TCP::EventFunction func) override
    {
        Mad<EspTCP> tcp = Mad<EspTCP>::create(MemoryType::Network);
        tcp->init(port, IPAddr(), func);
        return tcp;
    }
    
    virtual Mad<m8r::UDP> createUDP(uint16_t port, m8r::UDP::EventFunction) override
    {
        return Mad<m8r::UDP>();
    }

    static void timerFunc(void* arg)
    {
        TimerEntry* t = reinterpret_cast<TimerEntry*>(arg);
        t->cb();
        if (!t->repeat) {
            t->running = false;
        }
    }

    virtual int8_t startTimer(Duration duration, bool repeat, std::function<void()> cb) override
    {
        int8_t id = -1;
        
        for (int i = 0; i < NumTimers; ++i) {
            if (!_timers[i].running) {
                id = i;
                break;
            }
        }
        
        if (id < 0) {
            return id;
        }
        
        _timers[id].running = true;
        _timers[id].repeat = repeat;
        _timers[id].cb = std::move(cb);
        
        os_timer_disarm(&(_timers[id].timer));
        os_timer_setfn(&(_timers[id].timer), timerFunc, &(_timers[id]));
        os_timer_arm(&(_timers[id].timer), duration.ms(), repeat);

        return id;
    }
    
    virtual void stopTimer(int8_t id) override
    {
        if (id < 0 || id >= NumTimers || !_timers[id].running) {
            return;
        }
        
        os_timer_disarm(&(_timers[id].timer));
        _timers[id].running = false;
}

private:
    void turnWiFiOn()
    {
        wifi_fpm_do_wakeup();
        wifi_fpm_close();
        wifi_set_opmode(STATION_MODE);
        wifi_station_connect();
    }

    void turnWiFiOff()
    {
        wifi_station_disconnect();
        wifi_set_opmode(NULL_MODE);
        wifi_set_sleep_type(MODEM_SLEEP_T);
        wifi_fpm_open();
        wifi_fpm_do_sleep(0xFFFFFFF);
    }

	void startNetwork()
	{
        system()->setHeartrate(500ms);

        Serial.print("Starting WiFi");

		WiFiManager wifiManager;

		if (_needsNetworkReset) {
			_needsNetworkReset = false;
			wifiManager.resetSettings();			
		}
		
		wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
			Serial.printf("Entered config mode:ip=%s, ssid='%s'\n", WiFi.softAPIP().toString().c_str(), wifiManager->getConfigPortalSSID().c_str());
            system()->setHeartrate(200ms);
			_enteredConfigMode = true;
		});

		if (!wifiManager.autoConnect(ConfigPortalName)) {
			Serial.printf("*** Failed to connect and hit timeout\n");
		    ESP.reset();
			delay(1000);
		}
		
		if (_enteredConfigMode) {
			// If we've been in config mode, the network doesn't startup correctly, let's reboot
			ESP.reset();
			delay(1000);
		}

        WiFi.mode(WIFI_STA);
        WiFiMode_t currentMode = WiFi.getMode();
        system()->setHeartrate(1s);
		Serial.printf("Wifi connected, Mode=%s, IP=%s\n", wifiManager.getModeString(currentMode).c_str(), WiFi.localIP().toString().c_str());
	
		_enableNetwork = true;
	}

    bool _needsNetworkReset = false;
    bool _enteredConfigMode = false;
    bool _enableNetwork = false;
	
    m8r::LittleFS _fileSystem;
    EspGPIOInterface _gpio;

    struct TimerEntry
    {
        bool running = false;
        bool repeat = false;
        os_timer_t timer;
        std::function<void()> cb;
    };
    TimerEntry _timers[NumTimers];
};

int32_t SystemInterface::heapFreeSize()
{
    return static_cast<int32_t>(ESP.getFreeHeap());
}

extern uint64_t g_esp_os_us;

SystemInterface* SystemInterface::create() { return new EspSystemInterface(); }

static int lfs_flash_read(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, void *dst, lfs_size_t size)
{
    uint32_t addr = (block * LittleFS::BlockSize) + off + FS_PHYS_ADDR;
    return flash_hal_read(addr, size, static_cast<uint8_t*>(dst)) == FLASH_HAL_OK ? 0 : -1;
}

static int lfs_flash_write(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t addr = (block * LittleFS::BlockSize) + off + FS_PHYS_ADDR;
    const uint8_t *src = reinterpret_cast<const uint8_t *>(buffer);
    return (flash_hal_write(addr, size, src) == FLASH_HAL_OK) ? 0 : -1;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t addr = FS_PHYS_ADDR + (block * LittleFS::BlockSize);
    return flash_hal_erase(addr, LittleFS::BlockSize) == FLASH_HAL_OK ? 0 : -1;
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
