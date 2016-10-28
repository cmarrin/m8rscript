/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.

    - Neither the name of the <ORGANIZATION> nor the names of its
	  contributors may be used to endorse or promote products derived from
	  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "Esp.h"
#include "MStream.h"
#include "ExecutionUnit.h"
#include "SystemInterface.h"
#include "Application.h"
#include "FS.h"
#include "TCP.h"
#include "Shell.h"

extern "C" {
#include <gpio.h>
#include <osapi.h>
#include <user_interface.h>
}

#include <cstdarg>

extern "C" {
    int ets_putc(int);
    int ets_vprintf(int (*print_function)(int), const char * format, va_list arg) __attribute__ ((format (printf, 2, 0)));
}

static m8r::TCP* _shellTCP = nullptr;

void ICACHE_FLASH_ATTR runScript()
{
    m8r::FS* fs = m8r::FS::sharedFS();
    if (!fs->mount()) {
        esp_system()->printf(ROMSTR("ERROR: Mount failed, trying to format.\n"));
        fs->format();
    }

    esp_system()->printf(ROMSTR("\n*** m8rscript v0.1\n\n"));
    esp_system()->printf(ROMSTR("***** start - free ram:%d\n"), system_get_free_heap_size());
    
    m8r::Application application(esp_system());
    m8r::Error error;
    if (!application.load(error)) {
        error.showError(esp_system());
    } else {
        application.run([]{
            esp_system()->printf(ROMSTR("***** finished - free ram:%d\n"), system_get_free_heap_size());
        });
    }
}

static volatile os_timer_t gBlinkTimer;

void blinkTimerfunc(void *)
{
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
    {
        esp_system()->printf("Blink\n");
        gpio_output_set(0, BIT2, BIT2, 0);
    } else {
        gpio_output_set(BIT2, 0, BIT2, 0);
    }
}

class MyShell : public m8r::Shell, public m8r::TCPDelegate {
public:
    MyShell(uint16_t port) : _tcp(m8r::TCP::create(this, port)) { }
    
    // TCPDelegate
    virtual void TCPconnected(m8r::TCP*) override { connected(); }
    virtual void TCPdisconnected(m8r::TCP*) override { disconnected(); }
    
    virtual void TCPreceivedData(m8r::TCP* tcp, const char* data, uint16_t length) override
    {
        if (!received(data, length)) {
            tcp->disconnect();
        }
    }

    virtual void TCPsentData(m8r::TCP*) override { sendComplete(); }
    
    // Shell Delegate
    virtual void shellSend(const char* data, uint16_t size = 0) { _tcp->send(data, size); }
    virtual void setDeviceName(const char* name) { setDeviceName(name); }

private:
    m8r::TCP* _tcp;
};

MyShell* _shell;

void systemInitialized()
{
    _shell = new MyShell(22);
    runScript();
}

extern "C" void ICACHE_FLASH_ATTR user_init()
{
    initializeSystem(systemInitialized);
    
    // init gpio subsystem
    gpio_init();
    
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    gpio_output_set(0, BIT2, BIT2, 0);

    os_timer_disarm((os_timer_t*) &gBlinkTimer);
    os_timer_setfn((os_timer_t*) &gBlinkTimer, (os_timer_func_t *)blinkTimerfunc, NULL);
    os_timer_arm((os_timer_t*) &gBlinkTimer, 1000, 1);
}
