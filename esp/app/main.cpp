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

#include "Parser.h"
#include "Stream.h"
#include "CodePrinter.h"
#include "ExecutionUnit.h"
#include "SystemInterface.h"
#include "HardwareSerial.h"

extern "C" {
#include "stdio.h"
#include "uart.h"
#include <user_interface.h>
}

#define PARSE_FILE 0
#define PARSE_STRING 1
#define EXECUTE 1

class MySystemInterface : public m8r::SystemInterface
{
public:
    virtual void printf(const char* s, ...) const override
    {
        va_list args;
        va_start(args, s);
        ets_vprintf(ets_putc, s, args);
    }
    virtual int read() const override { return Serial.read(); }
};

void runScript() {
    const char* filename = "simple.m8r";

    MySystemInterface systemInterface;
    systemInterface.printf("\n*** m8rscript v0.1\n\n");

    systemInterface.printf("***** start - free ram:%d\n", system_get_free_heap_size());

    m8r::Program* program = nullptr;
    
#if PARSE_FILE
    systemInterface.printf("Opening '%s'\n", filename);
    m8r::FileStream istream(filename);
    if (!istream.loaded()) {
        systemInterface.printf("File not found, exiting\n");
        abort();
    }
#elif PARSE_STRING
    m8r::String fileString = 
"var a = [ ]; \n \
var n = 500; \n \
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
    systemInterface.printf("Parsing...\n");
    m8r::Parser parser(&systemInterface);
    parser.parse(&istream);
    systemInterface.printf("***** after parse - free ram:%d\n", system_get_free_heap_size());

    systemInterface.printf("Finished. %d error%s\n\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");

    if (!parser.nerrors()) {
        program = parser.program();
#elif EXECUTE
        m8r::Program _program(&systemInterface);
        program = &_program;
#endif
#if EXECUTE
        systemInterface.printf("\n***** Start of Program Output *****\n\n");
        m8r::ExecutionUnit eu(&systemInterface);
        eu.run(program);
        systemInterface.printf("\n***** End of Program Output *****\n\n");
#endif
#if PARSE_FILE || PARSE_STRING
    }
#endif
    systemInterface.printf("***** after run - free ram:%d\n", system_get_free_heap_size());
}
