/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Error.h"

using namespace m8r;

ROMString Error::description(Code code)
{
    switch(code) {
        case Code::OK                       : return ROMSTR("OK");
        case Code::Unknown                  : return ROMSTR("Unknown");
        case Code::Write                    : return ROMSTR("Write");
        case Code::Read                     : return ROMSTR("Read");
        case Code::SerialHeader             : return ROMSTR("Serial Header");
        case Code::SerialType               : return ROMSTR("Serial Type");
        case Code::SerialVersion            : return ROMSTR("Serial Version");
        case Code::FileNotFound             : return ROMSTR("File Not Found");
        case Code::FileClosed               : return ROMSTR("File Closed");
        case Code::ParseError               : return ROMSTR("Parse");
        case Code::RuntimeError             : return ROMSTR("Runtime");
        case Code::Exists                   : return ROMSTR("Exists");
        case Code::ReadError                : return ROMSTR("Read Error");
        case Code::WriteError               : return ROMSTR("Write Error");
        case Code::NotReadable              : return ROMSTR("Not Readable");
        case Code::NotWritable              : return ROMSTR("Not Writable");
        case Code::SeekNotAllowed           : return ROMSTR("Seek Not Allowed");
        case Code::TooManyOpenFiles         : return ROMSTR("Too Many Open Files");
        case Code::DirectoryNotFound        : return ROMSTR("Directory Not Found");
        case Code::DirectoryNotEmpty        : return ROMSTR("Directory Not Empty");
        case Code::NotADirectory            : return ROMSTR("Not A Directory");
        case Code::NotAFile                 : return ROMSTR("Not A File");
        case Code::InvalidFileName          : return ROMSTR("Invalid Filename");
        case Code::FSNotFormatted           : return ROMSTR("Filesystem Not Formatted");
        case Code::FormatFailed             : return ROMSTR("Filesystem Format Failed");
        case Code::NoFS                     : return ROMSTR("No Filesystem Found");
        case Code::NoSpace                  : return ROMSTR("No Space Left");
        case Code::MountFailed              : return ROMSTR("Mount Failed");
        case Code::NotMounted               : return ROMSTR("Not Mounted");
        case Code::Mounted                  : return ROMSTR("Already Mounted");
        case Code::Corrupted                : return ROMSTR("Corrupted");
        case Code::OutOfMemory              : return ROMSTR("Out of Memory");
        case Code::InternalError            : return ROMSTR("Internal");
        default                             : return ROMSTR("*** INVALID CODE ***");
    }
}

m8r::String Error::formatError(Code code, ROMString format, ...)
{
    va_list args;
    va_start(args, format);
    return vformatError(code, format, args);
}

m8r::String Error::formatError(Code code, int32_t lineno, ROMString format, ...)
{
    va_list args;
    va_start(args, format);
    return vformatError(code, lineno, format, args);
}

m8r::String Error::vformatError(Code code, ROMString format, va_list args)
{
    return vformatError(code, 0, format, args);
}

m8r::String Error::vformatError(Code code, int32_t lineno, ROMString format, va_list args)
{
    String s(description(code));
    s += String(ROMSTR(" Error: "));
    if (!format.valid()) {
        return s;
    }

    s += String(ROMSTR(": "));
    s += String::vformat(String(format).c_str(), args);
    if (lineno > 0) {
        s += String(ROMSTR(" on line ")) + String(lineno);
    }
    s += String(ROMSTR("\n"));
    return s;
}

