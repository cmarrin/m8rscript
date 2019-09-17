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

#include "Defines.h"
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
}

#include <cstdarg>

extern "C" {
    int ets_putc(int);
    int ets_vprintf(int (*print_function)(int), const char * format, va_list arg) __attribute__ ((format (printf, 2, 0)));
}

class MyTCP;

m8r::Application _application(fs(), system(), 22);

void FLASH_ATTR runScript()
{
    const char* dnsname = "time.nist.gov";
    m8r::IPAddr::lookupHostName(dnsname, [](const char* name, m8r::IPAddr ipaddr) {
        uint32_t ip = ipaddr;
        system()->printf(ROMSTR("\n************** DNS Lookup:'%s = %d.%d.%d.%d\n"), name, ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
    });

    system()->printf(ROMSTR("\n*** m8rscript v0.1\n\n"));
    m8r::MemoryInfo info;
    m8r::Object::memoryInfo(info);
    system()->printf(ROMSTR("***** start - free ram:%d, num allocations:%d\n"), info.freeSize, info.numAllocations);
    
    m8r::Error error;
    if (!_application.load(error, false)) {
        error.showError(system());
    } else if (!_application.program()) {
        system()->printf(ROMSTR("Error:failed to compile application"));
    } else {
        _application.run([]{
            m8r::MemoryInfo info;
            m8r::Object::memoryInfo(info);
            system()->printf(ROMSTR("***** finished - free ram:%d, num allocations:%d\n"), info.freeSize, info.numAllocations);
        });
    }
}

void systemInitialized()
{
    runScript();
}

extern "C" void FLASH_ATTR user_init()
{
    initializeSystem(systemInitialized);
}
