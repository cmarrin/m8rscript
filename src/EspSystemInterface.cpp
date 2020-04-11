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
#include "MFS.h"
#include "SystemInterface.h"
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

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

std::function<void()> _timerCB;

static void ICACHE_RAM_ATTR onTimerISR()
{
    _timerCB();
}

class EspFile: public m8r::File
{
    friend class EspFS;

public:
    virtual void close() override
    {
        if (_file) {
            _file.close();
        }
    }

    virtual int32_t read(char* buf, uint32_t size) override
    {
        return _file ? _file.readBytes(buf, size) : -1;
    }

    virtual int32_t write(const char* buf, uint32_t size) override
    {
        return _file ? _file.write(buf, size) : -1;
    }

    virtual  bool seek(int32_t offset, SeekWhence whence = SeekWhence::Set) override
    {
        auto w = SeekSet;
        if (whence == SeekWhence::Cur) {
            w = SeekCur;
        } else if (whence == SeekWhence::End) {
            w = SeekEnd;
        }
        return _file ? _file.seek(offset, w) : false;
    }

    virtual int32_t tell() const override { return _file ? _file.position() : -1; }
    virtual int32_t size() const override { return _file ? _file.size() : -1; }

private:
    ::File _file;
};

class EspFS : public m8r::FS
{
public:
    EspFS()
    {
        LittleFSConfig config;
        config.setAutoFormat(false);
        LittleFS.setConfig(config);
    }

    virtual ~EspFS() { }

    virtual bool mount() override
    {
        if (!LittleFS.begin()) {
            _error = Error::Code::FSNotFormatted;
            return false;
        }
        return true;
    }

    virtual bool mounted() const override { FSInfo info; return LittleFS.info(info); }
    virtual void unmount() override { LittleFS.end(); }
    virtual bool format() override
    {
        if (!LittleFS.format()) {
            _error = Error::Code::MountFailed;
            return false;
        }
        return true;
        return LittleFS.format();
    }
    
    virtual bool makeDirectory(const char* name) override { return LittleFS.mkdir(name); }
    virtual bool remove(const char* name) override { return LittleFS.remove(name); }
    virtual bool rename(const char* src, const char* dst) override { return LittleFS.rename(src, dst); }
    virtual bool exists(const char* name) const override { return LittleFS.exists(name); }
    
    virtual uint32_t totalSize() const override
    { 
        FSInfo info;
        if (!LittleFS.info(info)) {
            return 0;
        }
        return info.totalBytes;
    }

    virtual uint32_t totalUsed() const override
    { 
        FSInfo info;
        if (!LittleFS.info(info)) {
            return 0;
        }
        return info.usedBytes;
    }

    virtual Mad<m8r::File> open(const char* name, FileOpenMode mode) override
    {
        const char* m = "r";
        switch (mode) {
            case FileOpenMode::Read: m = "r"; break;
            case FileOpenMode::ReadUpdate: m = "r+"; break;
            case FileOpenMode::Write: m = "w"; break;
            case FileOpenMode::WriteUpdate: m = "w+"; break;
            case FileOpenMode::Append: m = "a"; break;
            case FileOpenMode::AppendUpdate: m = "a+"; break;
            case FileOpenMode::Create: m = "w+"; break;
        }

        // Create mode is like w+ if the file doesn't exist, and like r+ if it does
        if (mode == FileOpenMode::Create && LittleFS.exists(name)) {
            m = "r+";
        }
        Mad<EspFile> file = Mad<EspFile>::create(MemoryType::Native);
        file->_file = LittleFS.open(name, m);
        if (!file->_file) {
            file->_error = Error::Code::FileNotFound;
        } else {
            file->_error = Error::Code::OK;
            file->_type = m8r::File::Type::File;
            file->_mode = mode;
        }
        return file;
    }

    virtual Mad<Directory> openDirectory(const char* name) override
    {
        // FIXME: Implement
        return Mad<Directory>();
    }
};

class EspSystemInterface : public SystemInterface
{
public:
    EspSystemInterface()
    {
        delay(500);

        startNetwork();
        // Serial.print("Connecting to ");
        // Serial.println("marrin");
        // WiFi.mode(WIFI_STA);
        // WiFi.begin("marrin", "orion741");
        // while (WiFi.status() != WL_CONNECTED) {
        //     delay(500);
        //     Serial.print(".");
        // }

        // Serial.println("");
        // Serial.println("WiFi connected");
        // Serial.println("IP address: ");
        // Serial.println(WiFi.localIP());
    }
    
    virtual void vprintf(ROMString fmt, va_list args) const override
    {
        m8r::String s = m8r::String::vformat(fmt, args);
        ::printf(s.c_str());
    }
    
    virtual void setDeviceName(const char* name) { }
    
    virtual m8r::FS* fileSystem() override { return &_fileSystem; }
    virtual GPIOInterface* gpio() override { return nullptr; }
    
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
	
    EspFS _fileSystem;
//    RtosGPIOInterface _gpio;
};

extern uint64_t g_esp_os_us;

SystemInterface* SystemInterface::create() { return new EspSystemInterface(); }
