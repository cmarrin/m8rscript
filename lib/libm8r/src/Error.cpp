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

const char* Error::description(Code code)
{
    switch(code) {
        case Code::OK                       : return "OK";
        case Code::Unknown                  : return "Unknown";
        case Code::Write                    : return "Write";
        case Code::Read                     : return "Read";
        case Code::SerialHeader             : return "Serial Header";
        case Code::SerialType               : return "Serial Type";
        case Code::SerialVersion            : return "Serial Version";
        case Code::FileNotFound             : return "File Not Found";
        case Code::FileExists               : return "File Exists";
        case Code::FileClosed               : return "File Closed";
        case Code::ParseError               : return "Parse";
        case Code::RuntimeError             : return "Runtime";
        case Code::Exists                   : return "Exists";
        case Code::ReadError                : return "Read Error";
        case Code::WriteError               : return "Write Error";
        case Code::NotReadable              : return "Not Readable";
        case Code::NotWritable              : return "Not Writable";
        case Code::SeekNotAllowed           : return "Seek Not Allowed";
        case Code::TooManyOpenFiles         : return "Too Many Open Files";
        case Code::DirectoryNotFound        : return "Directory Not Found";
        case Code::DirectoryNotEmpty        : return "Directory Not Empty";
        case Code::NotADirectory            : return "Not A Directory";
        case Code::NotAFile                 : return "Not A File";
        case Code::InvalidFileName          : return "Invalid Filename";
        case Code::FSNotFormatted           : return "Filesystem Not Formatted";
        case Code::FormatFailed             : return "Filesystem Format Failed";
        case Code::NoFS                     : return "No Filesystem Found";
        case Code::NoSpace                  : return "No Space Left";
        case Code::MountFailed              : return "Mount Failed";
        case Code::NotMounted               : return "Not Mounted";
        case Code::Mounted                  : return "Already Mounted";
        case Code::Corrupted                : return "Corrupted";
        case Code::OutOfMemory              : return "Out of Memory";
        case Code::InternalError            : return "Internal";
        case Code::Unimplemented            : return "Unimplemented";
        case Code::WrongNumberOfParams      : return "Wrong Number of Params";
        case Code::PropertyDoesNotExist     : return "Property Does Not Exist";
        case Code::InvalidArgumentValue     : return "Invalid Argument Value";
        case Code::MissingThis              : return "Missing this";
        case Code::CannotCall               : return "Cannot Call";
        case Code::CannotConvertStringToNumber: return "Cannot Convert String to Number";
        case Code::OutOfRange               : return "Out of Range";
    }
    return "";
}

m8r::String Error::formatError(Code code, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    return vformatError(code, format, args);
}

m8r::String Error::formatError(Code code, int32_t lineno, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    return vformatError(code, lineno, format, args);
}

m8r::String Error::vformatError(Code code, const char* format, va_list args)
{
    return vformatError(code, 0, format, args);
}

m8r::String Error::vformatError(Code code, int32_t lineno, const char* format, va_list args)
{
    String s(description(code));
    s += String(" Error");
    if (!format) {
        return s;
    }

    s += String(": ");
    s += String::vformat(format, args);
    if (lineno > 0) {
        s += String(" on line ") + String(lineno);
    }
    s += String("\n");
    return s;
}

String ParseErrorEntry::format() const
{
    String s("***** ");
    s += Error::formatError(Error::Code::ParseError, _lineno, _description.c_str());
    return s;
}
