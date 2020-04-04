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
#include "MString.h"

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
        FileExists,
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
        NoFS,
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
    
    ROMString description() const;
    
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

struct ParseErrorEntry {
    ParseErrorEntry() { }
    ParseErrorEntry(const char* description, uint32_t lineno, uint16_t charno = 1, uint16_t length = 1)
        : _description(description)
        , _lineno(lineno)
        , _charno(charno)
        , _length(length)
    {
    }
    
    ParseErrorEntry(const ParseErrorEntry& other)
        : _description(other._description)
        , _lineno(other._lineno)
        , _charno(other._charno)
        , _length(other._length)
    {
    }
    
    ~ParseErrorEntry() { }
    
    String _description;
    uint32_t _lineno = 0;
    uint16_t _charno = 0;
    uint16_t _length = 0;
};

using ParseErrorList = Vector<ParseErrorEntry>;

}
