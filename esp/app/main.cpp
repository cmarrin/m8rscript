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

os_timer_t gExecutionTimer;
static const uint32_t ExecutionTaskPrio = 0;
static const uint32_t ExecutionTaskQueueLen = 1;
os_event_t gExecutionTaskQueue[ExecutionTaskQueueLen];

void ICACHE_FLASH_ATTR executionTask(os_event_t *event)
{
    m8r::ExecutionUnit* eu = reinterpret_cast<m8r::ExecutionUnit*>(event->par);
    int32_t delay = eu->continueExecution();
    if (delay == 0) {
        system_os_post(ExecutionTaskPrio, 0, event->par);
    } else if (delay > 0) {
        os_timer_arm(&gExecutionTimer, delay, false);
    } else {
        esp_system()->printf(ROMSTR("\n***** End of Program Output *****\n\n"));
        esp_system()->printf(ROMSTR("***** after run - free ram:%d\n"), system_get_free_heap_size());
    }
}

void ICACHE_FLASH_ATTR executionTimerTick(void* data)
{
    system_os_post(ExecutionTaskPrio, 0, reinterpret_cast<uint32_t>(data));
}

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
        m8r::Program* program = application.program();
    
        esp_system()->printf(ROMSTR("\n***** Start of Program Output *****\n\n"));
        m8r::ExecutionUnit* eu = new m8r::ExecutionUnit(esp_system());
        eu->startExecution(program);

        os_timer_disarm(&gExecutionTimer);
        os_timer_setfn(&gExecutionTimer, (os_timer_func_t*) &executionTimerTick, eu);

        // Fire the execution task directly (0 timeout)
        system_os_post(ExecutionTaskPrio, 0, reinterpret_cast<uint32_t>(eu));
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

class MyShellTCP : public m8r::TCP, public m8r::ShellOutput {
public:
    MyShellTCP(uint16_t port) : m8r::TCP(port), _shell(this) { }
    
    virtual void connected() override { _shell.connected(); }
    
    virtual void disconnected() override { _shell.disconnected(); }
    
    virtual void receivedData(const char* data, uint16_t length) override
    {
        if (!_shell.received(data, length)) {
            disconnect();
        }
    }

    virtual void sentData() override { _shell.sendComplete(); }

    virtual void shellSend(const char* data, uint16_t size = 0) { send(data, size); }
    virtual void setDeviceName(const char* name) { setDeviceName(name); }

private:
    m8r::Shell _shell;
};

void systemInitialized()
{
    _shellTCP = new MyShellTCP(22);
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

    system_os_task(executionTask, ExecutionTaskPrio, gExecutionTaskQueue, ExecutionTaskQueueLen);
}
