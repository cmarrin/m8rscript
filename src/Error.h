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
        NotADirectory,
        NotAFile,
        InvalidFileName,
        FSNotFormatted,
        NoSpace,
        MountFailed,
        NotMounted,
        Mounted,
        InternalError,
     };
    
    Error() { }
    ~Error() { }
    
    operator bool () const { return _code == Code::OK; }
    Error& operator= (const Error::Code& code) { _code = code; return *this; }
    bool operator==(const Error::Code& code) const { return _code == code; }
    bool operator!=(const Error::Code& code) const { return _code != code; }

    Code code() const { return _code; }
    
    static void showError(Code);
    
    static void printError(Code code, const char* format = nullptr, ...);
    static void printError(Code, int32_t lineno, const char* format = nullptr, ...);
    static void vprintError(Code, const char* format, va_list);
    static void vprintError(Code, int32_t lineno, const char* format, va_list);

private:
    Code _code = Code::OK;
};

}
