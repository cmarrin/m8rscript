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

void Error::showError(Code code)
{
    const char* codeString;
    switch(code) {
        case Code::OK                       : codeString = ROMSTR("OK"); break;
        case Code::Unknown                  : codeString = ROMSTR("Unknown"); break;
        case Code::Write                    : codeString = ROMSTR("Write"); break;
        case Code::Read                     : codeString = ROMSTR("Read"); break;
        case Code::SerialHeader             : codeString = ROMSTR("Serial Header"); break;
        case Code::SerialType               : codeString = ROMSTR("Serial Type"); break;
        case Code::SerialVersion            : codeString = ROMSTR("Serial Version"); break;
        case Code::FileNotFound             : codeString = ROMSTR("File Not Found"); break;
        case Code::ParseError               : codeString = ROMSTR("Parse"); break;
        case Code::RuntimeError             : codeString = ROMSTR("Runtime"); break;
        case Code::Exists                   : codeString = ROMSTR("Exists"); break;
        case Code::ReadError                : codeString = ROMSTR("Read Error"); break;
        case Code::WriteError               : codeString = ROMSTR("Write Error"); break;
        case Code::NotReadable              : codeString = ROMSTR("Not Readable"); break;
        case Code::NotWritable              : codeString = ROMSTR("Not Writable"); break;
        case Code::SeekNotAllowed           : codeString = ROMSTR("Seek Not Allowed"); break;
        case Code::TooManyOpenFiles         : codeString = ROMSTR("Too Many Open Files"); break;
        case Code::DirectoryNotFound        : codeString = ROMSTR("Directory Not Found"); break;
        case Code::NotADirectory            : codeString = ROMSTR("Not A Directory"); break;
        case Code::NotAFile                 : codeString = ROMSTR("Not A File"); break;
        case Code::InvalidFileName          : codeString = ROMSTR("Invalid Filename"); break;
        case Code::FSNotFormatted           : codeString = ROMSTR("Fs Not Formatted"); break;
        case Code::NoSpace                  : codeString = ROMSTR("No Space Left"); break;
        case Code::MountFailed              : codeString = ROMSTR("Mount Failed"); break;
        case Code::NotMounted               : codeString = ROMSTR("Not Mounted"); break;
        case Code::Mounted                  : codeString = ROMSTR("Already Mounted"); break;
        case Code::InternalError            : codeString = ROMSTR("Internal Error"); break;
    }
    system()->printf(codeString);
}

void Error::printError(Code code, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintError(code, format, args);
}

void Error::printError(Code code, int32_t lineno, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintError(code, lineno, format, args);
}

void Error::vprintError(Code code, const char* format, va_list args)
{
    vprintError(code, 0, format, args);
}

void Error::vprintError(Code code, int32_t lineno, const char* format, va_list args)
{
    showError(code);
    system()->printf(ROMSTR(" Error"));
    if (!format) {
        return;
    }
    system()->printf(ROMSTR(": "));
    system()->vprintf(format, args);
    if (lineno > 0) {
        system()->printf(ROMSTR(" on line %d"), lineno);
    }
    system()->printf(ROMSTR("\n"));
}

