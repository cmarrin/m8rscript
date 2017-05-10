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

#include "GPIOInterface.h"
#include <cstdint>
#include <cstdarg>
#include <vector>

namespace m8r {

struct ErrorEntry {
    ErrorEntry(const char* description, uint32_t lineno, uint16_t charno = 1, uint16_t length = 1)
        : _lineno(lineno)
        , _charno(charno)
        , _length(length)
    {
        size_t size = strlen(description);
        _description = new char[size + 1];
        memcpy(_description, description, size + 1);
    }
    
    ErrorEntry(const ErrorEntry& other)
        : _lineno(other._lineno)
        , _charno(other._charno)
        , _length(other._length)
    {
        size_t size = strlen(other._description);
        _description = new char[size + 1];
        memcpy(_description, other._description, size + 1);
    }
    
    ~ErrorEntry()
    {
        delete [ ] _description;
    }
    
    char* _description;
    uint32_t _lineno;
    uint16_t _charno;
    uint16_t _length;
};

typedef std::vector<ErrorEntry> ErrorList;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SystemInterface
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class SystemInterface  {
public:
	SystemInterface() { }
    virtual ~SystemInterface() { }

    void printf(const char* fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
    }
    
    enum class MemoryType { Object };
    static void* alloc(MemoryType, size_t);
    static void free(MemoryType, void*);
    
    virtual void vprintf(const char*, va_list) const = 0;
    virtual GPIOInterface& gpio() = 0;
    virtual uint32_t freeMemory() const = 0;
    static uint64_t currentMicroseconds();
};

}
