/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Error.h"

#include "ExecutionUnit.h"

using namespace m8r;

void Error::showError(const ExecutionUnit* eu, Code code)
{
    ROMString codeString = ROMSTR("*** INVALID CODE ***");
    switch(code) {
        case Code::OK                       : codeString = ROMSTR("OK"); break;
        case Code::Unknown                  : codeString = ROMSTR("Unknown"); break;
        case Code::Write                    : codeString = ROMSTR("Write"); break;
        case Code::Read                     : codeString = ROMSTR("Read"); break;
        case Code::SerialHeader             : codeString = ROMSTR("Serial Header"); break;
        case Code::SerialType               : codeString = ROMSTR("Serial Type"); break;
        case Code::SerialVersion            : codeString = ROMSTR("Serial Version"); break;
        case Code::FileNotFound             : codeString = ROMSTR("File Not Found"); break;
        case Code::FileClosed               : codeString = ROMSTR("File Closed"); break;
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
        case Code::DirectoryNotEmpty        : codeString = ROMSTR("Directory Not Empty"); break;
        case Code::NotADirectory            : codeString = ROMSTR("Not A Directory"); break;
        case Code::NotAFile                 : codeString = ROMSTR("Not A File"); break;
        case Code::InvalidFileName          : codeString = ROMSTR("Invalid Filename"); break;
        case Code::FSNotFormatted           : codeString = ROMSTR("Fs Not Formatted"); break;
        case Code::NoSpace                  : codeString = ROMSTR("No Space Left"); break;
        case Code::MountFailed              : codeString = ROMSTR("Mount Failed"); break;
        case Code::NotMounted               : codeString = ROMSTR("Not Mounted"); break;
        case Code::Mounted                  : codeString = ROMSTR("Already Mounted"); break;
        case Code::Corrupted                : codeString = ROMSTR("Corrupted"); break;
        case Code::OutOfMemory              : codeString = ROMSTR("Out of Memory"); break;
        case Code::InternalError            : codeString = ROMSTR("Internal Error"); break;
    }
    
    if (eu) {
        eu->printf(codeString);
    } else {
        system()->printf(codeString);
    }
}

void Error::printError(const ExecutionUnit* eu, Code code, ROMString format, ...)
{
    if (!format.valid()) {
        showError(eu, code);
        eu->printf(ROMSTR("\n"));
        return;
    }
    
    va_list args;
    va_start(args, format);
    vprintError(eu, code, format, args);
}

void Error::printError(const ExecutionUnit* eu, Code code, int32_t lineno, ROMString format, ...)
{
    va_list args;
    va_start(args, format);
    vprintError(eu, code, lineno, format, args);
}

void Error::vprintError(const ExecutionUnit* eu, Code code, ROMString format, va_list args)
{
    vprintError(eu, code, 0, format, args);
}

void Error::vprintError(const ExecutionUnit* eu, Code code, int32_t lineno, ROMString format, va_list args)
{
    showError(eu, code);
    eu->printf(ROMSTR(" Error"));
    if (!format.valid()) {
        return;
    }
    eu->printf(ROMSTR(": "));
    eu->vprintf(format, args);
    if (lineno > 0) {
        eu->printf(ROMSTR(" on line %d"), lineno);
    }
    eu->printf(ROMSTR("\n"));
}

