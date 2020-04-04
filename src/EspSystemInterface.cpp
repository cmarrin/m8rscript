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
#include "SystemInterface.h"
#include "Esp.h"

using namespace m8r;

static constexpr uint32_t FSStart = 0x100000;

int32_t m8r::heapFreeSize()
{
    return static_cast<int32_t>(ESP.getFreeHeap());
}

void IPAddr::lookupHostName(const char* name, std::function<void (const char* name, IPAddr)> func)
{
}

std::function<void()> _timerCB;

void ICACHE_RAM_ATTR onTimerISR()
{
    _timerCB();
}

class EspSystemInterface : public SystemInterface
{
public:
    EspSystemInterface()
    {
    }
    
    virtual void vprintf(ROMString fmt, va_list args) const override
    {
        m8r::String s = m8r::String::vformat(fmt, args);
        ::printf(s.c_str());
    }
    
    virtual void setDeviceName(const char* name) { }
    
    virtual FS* fileSystem() override { return nullptr /*&_fileSystem*/; }
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

    virtual void startTimer(Duration duration, std::function<void()> cb) override
    {
        timer1_attachInterrupt(onTimerISR);
        timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
        timer1_write(duration.us());
    }
    
    virtual void stopTimer() override
    {
        timer1_disable();
    }

private:
//    RtosGPIOInterface _gpio;
};

extern uint64_t g_esp_os_us;

SystemInterface* SystemInterface::create() { return new EspSystemInterface(); }
