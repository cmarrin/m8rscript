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

#ifdef __APPLE__
#include <cstdio>
#endif

#include "Containers.h"

#include "SystemInterface.h"

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Stream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class Stream {
public:
	virtual int available() const = 0;
    virtual int read() const = 0;
    virtual int write(uint8_t) = 0;
	virtual void flush() = 0;
	
private:
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: FileStream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class FileStream : public m8r::Stream {
#ifdef __APPLE__
public:
	FileStream(const char* file, const char* mode = "r")
    {
        _file = fopen(file, mode);
        if (!_file) {
            _size = 0;
            return;
        }
        fseek(_file, 0, SEEK_END);
        _size = ftell(_file);
        rewind(_file);
    }
    ~FileStream()
    {
        if (_file) {
            fclose(_file);
            _file = nullptr;
        }
    }
	
    bool loaded()
    {
        return _file;
    }
	virtual int available() const override
    {
        return static_cast<int>(_size - ftell(_file));
    }
    virtual int read() const override
    {
        return fgetc(_file);
    }
    virtual int write(uint8_t c) override
    {
        return fputc(c, _file);
    }
	virtual void flush() override { }
	
private:
    FILE* _file;
    size_t _size;
#else
public:
	FileStream(const char* file)
    {
    }
	
    bool loaded()
    {
        return true;
    }
	virtual int available() const override
    {
        return 0;
    }
    virtual int read() const override
    {
        return 0;
    }
    virtual int write(uint8_t c) override
    {
        return 0;
    }
	virtual void flush() override { }
	
private:

#endif
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: StringStream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class StringStream : public m8r::Stream {
public:
	StringStream(const String& s) : _string(s), _index(0) { }
	StringStream(const char* s) : _string(s), _index(0) { }
    
    virtual ~StringStream() { }
	
    bool loaded() { return true; }
	virtual int available() const override
    {
        return static_cast<int>(_string.length() - _index);
    }
    virtual int read() const override
    {
        return (_index < _string.length()) ? _string[_index++] : -1;
    }
    virtual int write(uint8_t c) override
    {
        // Only allow writing to the end of the string
        if (_index != _string.length()) {
            return -1;
        }
        _string += c;
        _index++;
        return c;
    }
	virtual void flush() override { }
	
private:
    String _string;
    mutable uint32_t _index;
};

}
