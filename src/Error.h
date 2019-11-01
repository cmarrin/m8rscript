/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"

namespace m8r {

class SystemInterface;

class Error
{
public:
    enum class Level { Error, Warning, Info };
    
    enum class Code {
        OK,
        Unknown,
        Write,
        Read,
        SerialHeader,
        SerialType,
        SerialVersion,
        FileNotFound,
        FileClosed,
        ParseError,
        RuntimeError,
        Exists,
        ReadError,
        WriteError,
        NotReadable,
        NotWritable,
        SeekNotAllowed,
        TooManyOpenFiles,
        DirectoryNotFound,
        DirectoryNotEmpty,
        NotADirectory,
        NotAFile,
        InvalidFileName,
        FSNotFormatted,
        NoSpace,
        MountFailed,
        NotMounted,
        Mounted,
        Corrupted,
        OutOfMemory,
        InternalError,
     };
    
    Error() { }
    Error(const Error::Code& code) : _code(code) { }
    ~Error() { }
    
    operator bool () const { return _code == Code::OK; }
    Error& operator= (const Error::Code& code) { _code = code; return *this; }
    bool operator==(const Error::Code& code) const { return _code == code; }
    bool operator!=(const Error::Code& code) const { return _code != code; }

    Code code() const { return _code; }
    
    static void showError(Error error) { showError(error.code()); }
    static void showError(Code);
    
    static void printError(Code code, const char* format = nullptr, ...);
    static void printError(Code, int32_t lineno, const char* format = nullptr, ...);
    static void vprintError(Code, const char* format, va_list);
    static void vprintError(Code, int32_t lineno, const char* format, va_list);

private:
    Code _code = Code::OK;
};

}
