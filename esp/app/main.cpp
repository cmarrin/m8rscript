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
#include "ExecutionUnit.h"
#include "FS.h"
#include "MStream.h"
#include "Shell.h"
#include "SystemInterface.h"
#include "TaskManager.h"
#include "TCP.h"

extern "C" {
#include <gpio.h>
#include <osapi.h>
#include <user_interface.h>

#ifndef NDEBUG
#include <gdbstub.h>
#endif
}

#include <cstdarg>

extern "C" {
    int ets_putc(int);
    int ets_vprintf(int (*print_function)(int), const char * format, va_list arg) __attribute__ ((format (printf, 2, 0)));
}

class MyShell : public m8r::Shell, public m8r::TCPDelegate {
public:
    MyShell(uint16_t port) : _tcp(m8r::TCP::create(this, port)) { }
    
    // TCPDelegate
    virtual void TCPconnected(m8r::TCP*, uint16_t connectionId) override { connected(); }
    virtual void TCPdisconnected(m8r::TCP*, uint16_t connectionId) override { disconnected(); }
    
    virtual void TCPreceivedData(m8r::TCP* tcp, uint16_t connectionId, const char* data, uint16_t length) override
    {
        if (!received(data, length)) {
            tcp->disconnect();
        }
    }

    virtual void TCPsentData(m8r::TCP*, uint16_t connectionId) override { sendComplete(); }
    
    // Shell Delegate
    virtual void shellSend(const char* data, uint16_t size = 0) { _tcp->send(data, size); }
    virtual void setDeviceName(const char* name) { ::setDeviceName(name); }

private:
    m8r::TCP* _tcp;
};

MyShell* _shell;

void ICACHE_FLASH_ATTR runScript()
{
    m8r::SystemInterface::shared()->printf(ROMSTR("\n*** m8rscript v0.1\n\n"));
    m8r::SystemInterface::shared()->printf(ROMSTR("***** start - free ram:%d\n"), system_get_free_heap_size());
    _shell->load(nullptr);
    _shell->run([]{
        m8r::SystemInterface::shared()->printf(ROMSTR("***** finished - free ram:%d\n"), system_get_free_heap_size());
    });
}

void systemInitialized()
{
    _shell = new MyShell(22);
    runScript();
}

extern "C" void ICACHE_FLASH_ATTR user_init()
{
    initializeSystem(systemInitialized);
}
