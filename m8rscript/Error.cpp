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

#include "Error.h"

#include "SystemInterface.h"

using namespace m8r;

void Error::showError(SystemInterface* system) const
{
    const char* codeString = "unknown";
    switch(_code) {
        default: break;
    }
    system->printf(ROMSTR("Error: %s\n"), codeString);
}

void Error::printError(SystemInterface* system, Code code, int32_t lineno, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintError(system, code, lineno, format, args);
}

void Error::vprintError(SystemInterface* system, Code code, int32_t lineno, const char* format, va_list args)
{
    const char* codeString = "";
    
    switch(code) {
        case Code::None: break;
        case Code::Unknown: codeString = ROMSTR("Unknown"); break;
        case Code::Write: codeString = ROMSTR("Write"); break;
        case Code::Read: codeString = ROMSTR("Read"); break;
        case Code::SerialHeader: codeString = ROMSTR("Serial Header"); break;
        case Code::SerialType: codeString = ROMSTR("Serial Type"); break;
        case Code::SerialVersion: codeString = ROMSTR("Serial Version"); break;
        case Code::FileNotFound: codeString = ROMSTR("File Not Found"); break;
        case Code::ParseError: codeString = ROMSTR("Parse"); break;
        case Code::RuntimeError: codeString = ROMSTR("Runtime"); break;
    }
    
    system->printf(codeString);
    system->printf(ROMSTR(" Error"));
    if (!format) {
        return;
    }
    system->printf(ROMSTR(": "));
    system->vprintf(format, args);
    if (lineno >= 0) {
        system->printf(ROMSTR(" on line %d"), lineno);
    }
    system->printf(ROMSTR("\n"));
}

