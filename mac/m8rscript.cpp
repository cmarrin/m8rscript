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

#include <iostream>
#include <unistd.h>
#include <ctime>

#include "Stream.h"
#include "Parser.h"
#include "CodePrinter.h"
#include "ExecutionUnit.h"
#include "SystemInterface.h"
#include "printf.h"

void tfp_putchar(char c)
{
    std::cout << c;
}

class MySystemInterface : public m8r::SystemInterface
{
public:
    virtual void printf(const char* s, ...) const override
    {
        va_list args;
        va_start(args, s);
        tfp_vprintf(s, args);
    }
    virtual int read() const override
    {
        return std::cin.get();
    }
    virtual void updateGPIOState(uint16_t mode, uint16_t state) override { std::cout << "mode=" << std::hex << mode << " state=" << std::hex << state << "\n"; }
};

class InputStream : public m8r::Stream
{
public:
    InputStream(m8r::SystemInterface* system) : _system(system) { }

    virtual ~InputStream() { }
	
	virtual int available() override
    {
        return 1;
    }
    virtual int read() override { return _system->read(); }
	virtual void flush() override { }
    
    void interactive();
	
private:
    m8r::SystemInterface* _system;
};

int main(int argc, const char* argv[])
{
    MySystemInterface system;
    std::clock_t startTime;
    std::clock_t parseTime;
    m8r::Parser parser(&system);

    if (argc < 2) {
        system.printf("No file specified, using stdin\n");
        InputStream istream(&system);
        std::clock_t startTime = std::clock();
        parser.parse(&istream);
        parseTime = std::clock() - startTime;
    } else {
//        m8r::FileStream istream(argv[1]);
//        system.printf("Opening '%s'\n", argv[1]);
//
//        if (!istream.loaded()) {
//            system.printf("File not found, exiting\n");
//            return 0;
//        }
            





    m8r::String fileString = 
"var a = [ ]; \n \
var n = 100; \n \
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





        system.printf("Parsing...\n");
        
        startTime = std::clock();
        parser.parse(&istream);
        parseTime = std::clock() - startTime;
    }
    
    system.printf("Finished. %d error%s\n\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
    std::clock_t printTime = 0;
    std::clock_t runTime = 0;
    
    if (!parser.nerrors()) {
        m8r::CodePrinter codePrinter(&system);
        startTime = std::clock();
        m8r::String s = codePrinter.generateCodeString(parser.program());
        printTime = std::clock() - startTime;
        system.printf("\n***** Start of Generated Code *****\n%s\n***** End of Generated Code *****\n", s.c_str());
        
        system.printf("\n***** Start of Program Output *****\n\n");
        m8r::ExecutionUnit eu(&system);
        startTime = std::clock();
        eu.run(parser.program());
        runTime = std::clock() - startTime;
        system.printf("\n***** End of Program Output *****\n");
    }
    
    system.printf("\n\nTiming results:\n");
    system.printf("    Parse:, %gus\n", static_cast<double>(parseTime) / CLOCKS_PER_SEC * 1000000);
        
    if (!parser.nerrors()) {
        system.printf("    Print:%gus\n", static_cast<double>(printTime) / CLOCKS_PER_SEC * 1000000);
        system.printf("    Run  :%gms\n", static_cast<double>(runTime) / CLOCKS_PER_SEC * 1000);
    }
    system.printf("\n");
    return 0;
}
