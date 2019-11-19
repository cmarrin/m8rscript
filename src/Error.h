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

class ExecutionUnit;

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
    
    friend bool operator==(const Error& a, Error::Code b) { return a._code == b; }
    friend bool operator==(Error::Code b, const Error& a) { return a._code == b; }
    friend bool operator!=(const Error& a, Error::Code b) { return a._code != b; }
    friend bool operator!=(Error::Code b, const Error& a) { return a._code != b; }

    Code code() const { return _code; }
    
    static void showError(Error error) { showError(error.code()); }
    static void showError(Code code) { showError(nullptr, code); }
    static void showError(const ExecutionUnit* eu, Error error) { showError(eu, error.code()); }
    static void showError(const ExecutionUnit*, Code);

    static void printError(const ExecutionUnit*, Code code, ROMString format = ROMString(), ...);
    static void printError(const ExecutionUnit*, Code, int32_t lineno, ROMString format = ROMString(), ...);
    static void vprintError(const ExecutionUnit*, Code, ROMString format, va_list);
    static void vprintError(const ExecutionUnit*, Code, int32_t lineno, ROMString format, va_list);

private:
    Code _code = Code::OK;
};

}
