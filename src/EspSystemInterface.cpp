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
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include "flash_hal.h"

using namespace m8r;

static constexpr const char* ConfigPortalName = "m8rscript";
static constexpr uint32_t FSStart = 0x100000;

int32_t m8r::heapFreeSize()
{
    return static_cast<int32_t>(ESP.getFreeHeap());
}

void IPAddr::lookupHostName(const char* name, std::function<void (const char* name, IPAddr)> func)
{
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

std::function<void()> _timerCB;

static void ICACHE_RAM_ATTR onTimerISR()
{
    _timerCB();
}

class EspSystemInterface : public SystemInterface
{
public:
    EspSystemInterface()
    {
        delay(500);

        startNetwork();
    }
    
    virtual void vprintf(ROMString fmt, va_list args) const override
    {
        m8r::String s = m8r::String::vformat(fmt, args);
        ::printf(s.c_str());
    }
    
    virtual void setDeviceName(const char* name) { }
    
    virtual m8r::FS* fileSystem() override { return &_fileSystem; }
    virtual GPIOInterface* gpio() override { return &_gpio; }
    
    virtual Mad<TCP> createTCP(uint16_t port, m8r::IPAddr ip, TCP::EventFunction) override
    {
        return Mad<TCP>();
    }
    
    virtual Mad<TCP> createTCP(uint16_t port, TCP::EventFunction) override
    {
        return Mad<TCP>();
    }
    
    virtual Mad<m8r::UDP> createUDP(uint16_t port, m8r::UDP::EventFunction) override
    {
        return Mad<m8r::UDP>();
    }

    virtual void startTimer(Duration duration, std::function<void()> cb) override
    {
        _timerCB = cb;
        timer1_attachInterrupt(onTimerISR);
        timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
        timer1_write(duration.us());
    }
    
    virtual void stopTimer() override
    {
        timer1_disable();
    }

private:
	void startNetwork()
	{
        delay(500);
        WiFi.mode(WIFI_STA);
        delay(500);
		WiFiManager wifiManager;

		if (_needsNetworkReset) {
			_needsNetworkReset = false;
			wifiManager.resetSettings();			
		}
		
		wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
			Serial.printf("Entered config mode:ip=%s, ssid='%s'\n", WiFi.softAPIP().toString().c_str(), wifiManager->getConfigPortalSSID().c_str());
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
		Serial.printf("Wifi connected, Mode=%s, IP=%s\n", wifiManager.getModeString(currentMode).c_str(), WiFi.localIP().toString().c_str());
	
		_enableNetwork = true;
		//_blinker.setRate(ConnectedRate);

		delay(500);
		//_stateMachine.sendInput(Input::Connected);
	}

    bool _needsNetworkReset = false;
    bool _enteredConfigMode = false;
    bool _enableNetwork = false;
	
    m8r::LittleFS _fileSystem;
    EspGPIOInterface _gpio;
};

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
