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
#include "Parser.h"
#include "MStream.h"
#include "CodePrinter.h"
#include "ExecutionUnit.h"
#include "SystemInterface.h"
#include "FS.h"

extern "C" {
#include <gpio.h>
#include <user_interface.h>
}

#include <cstdarg>

extern "C" {
    int ets_putc(int);
    int ets_vprintf(int (*print_function)(int), const char * format, va_list arg) __attribute__ ((format (printf, 2, 0)));
}

#define PARSE_FILE 0
#define PARSE_STRING 1

class MySystemInterface : public m8r::SystemInterface
{
public:
    virtual void printf(const char* s, ...) const override
    {
        va_list args;
        va_start(args, s);
        ets_vprintf(ets_putc, s, args);
    }
    virtual int read() const override { return readSerialChar(); }
};

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
        os_printf("\n***** End of Program Output *****\n\n");
        os_printf("***** after run - free ram:%d\n", system_get_free_heap_size());
    }
}

void ICACHE_FLASH_ATTR executionTimerTick(void* data)
{
    system_os_post(ExecutionTaskPrio, 0, reinterpret_cast<uint32_t>(data));
}

void runScript()
{
    initializeSystem();

    esp::FS* fs = esp::FS::sharedFS();
    if (!fs->mount()) {
        os_printf("ERROR: Mount failed, trying to format.\n");
        fs->format();
        fs->mount();
    }
    if (fs->mounted()) {
//        esp::File* f = fs->open("FileA", SPIFFS_CREAT | SPIFFS_WRONLY);
//        if (!f->valid()) {
//            os_printf("ERROR: Failed to open 'FileA' for write, error=%d\n", f->error());
//        }
//        if (!f->write("abc", 3)) {
//            os_printf("ERROR: Failed to write to 'FileA', error=%d\n", f->error());
//        }
//        delete f;
        os_printf("Files {\n");
        esp::DirectoryEntry* entry = fs->directory();
        while (entry && entry->valid()) {
            uint32_t size = entry->size();
            os_printf("    '%s':%d bytes\n", entry->name(), size);
            
            esp::File* f = fs->open("FileA", SPIFFS_RDONLY);
            if (!f->valid()) {
                os_printf("ERROR: Failed to open 'FileA' for read, error=%d\n", f->error());
            } else {
                char* buf = new char[size];
                int32_t result = f->read(buf, size);
                if (result != size) {
                    os_printf("ERROR: Failed to read from 'FileA', error=%d\n", result);
                } else {
                    os_printf("        bytes:");
                    for (int i = 0; i < size; ++i) {
                        os_printf("0x%02x ", buf[i]);
                    }
                    os_printf("\n");
                }
                delete buf;
            }
            
            delete f;
      
            entry->next();
        }
        os_printf("}\n");
        if (entry) {
            delete entry;
        }
    }
    
    MySystemInterface* systemInterface = new MySystemInterface();

    const char* filename = "simple.m8r";

    os_printf("\n*** m8rscript v0.1\n\n");

    os_printf("***** start - free ram:%d\n", system_get_free_heap_size());

    m8r::Program* program = nullptr;
    
#if PARSE_FILE
    os_printf("Opening '%s'\n", filename);
    m8r::FileStream istream(filename);
    if (!istream.loaded()) {
        os_printf("File not found, exiting\n");
        abort();
    }
#elif PARSE_STRING
    m8r::String fileString = 
"var a = [ ]; \n \
var n = 200; \n \
 \n \
var startTime = Date.now(); \n \
 \n \
for (var i = 0; i < n; ++i) { \n \
    for (var j = 0; j < n; ++j) { \n \
        var f = 1.5; \n \
        a[j] = 1.5 * j * (j + 1) / 2; \n \
    } \n \
} \n \
 \n \
var t = Date.now() - startTime; \n \
Serial.print(\"Run time: \" + (t * 1000.) + \"ms\n\"); \n \
";
    
    m8r::StringStream istream(fileString);
#endif

#if PARSE_FILE || PARSE_STRING
    systemInterface->printf("Parsing...\n");
    m8r::Parser parser(systemInterface);
    parser.parse(&istream);
    os_printf("***** after parse - free ram:%d\n", system_get_free_heap_size());

    os_printf("Finished. %d error%s\n\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");

    if (!parser.nerrors()) {
        program = parser.program();
#else
        m8r::Program _program(&systemInterface);
        program = &_program;
#endif
        os_printf("\n***** Start of Program Output *****\n\n");
        m8r::ExecutionUnit* eu = new m8r::ExecutionUnit(systemInterface);
        eu->startExecution(program);

        os_timer_disarm(&gExecutionTimer);
        os_timer_setfn(&gExecutionTimer, (os_timer_func_t*) &executionTimerTick, eu);

        // Fire the execution task directly (0 timeout)
        system_os_post(ExecutionTaskPrio, 0, reinterpret_cast<uint32_t>(eu));
        
        os_timer_arm(&gExecutionTimer, 10, false);
#if PARSE_FILE || PARSE_STRING
    }
    os_printf("***** after run - free ram:%d\n", system_get_free_heap_size());
#endif
}

static volatile os_timer_t gBlinkTimer;

void blinkTimerfunc(void *)
{
    static int holdoff = 2;

    if (holdoff != 0) {
        if (--holdoff == 0) {
//            struct softap_config config;
//            if (wifi_softap_get_config_default(&config)) {
//                os_printf("wifi_softap_get_config:\n");
//                os_printf("    ssid = %s\n", config.ssid);
//                os_printf("    password = %s\n", config.password);
//                os_printf("    channel = %d\n", config.channel);
//            } else {
//                os_printf("wifi_softap_get_config FAILED\n");
//            }
            
            runScript();
        }
    }

    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
    {
        gpio_output_set(0, BIT2, BIT2, 0);
    } else {
        gpio_output_set(BIT2, 0, BIT2, 0);
    }
}

void initDone()
{
    initializeSystem();
    
    // init gpio subsystem
    gpio_init();
    
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    gpio_output_set(0, BIT2, BIT2, 0);

    os_timer_disarm((os_timer_t*) &gBlinkTimer);
    os_timer_setfn((os_timer_t*) &gBlinkTimer, (os_timer_func_t *)blinkTimerfunc, NULL);
    os_timer_arm((os_timer_t*) &gBlinkTimer, 1000, 1);

    system_os_task(executionTask, ExecutionTaskPrio, gExecutionTaskQueue, ExecutionTaskQueueLen);
}

#ifdef USE_SMING
void ICACHE_FLASH_ATTR maininit()
#else
extern "C" void ICACHE_FLASH_ATTR user_init()
#endif
{
    system_init_done_cb(initDone);
}