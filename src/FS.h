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
#include "Error.h"
#include <cstdint>
#include <memory>

namespace m8r {

class File;
class Directory;

class FS {
    friend class File;
    
public:
    enum class FileOpenMode { Read, ReadUpdate, Write, WriteUpdate, Append, AppendUpdate };
    
    FS() { }
    virtual ~FS() { }

    virtual bool mount() = 0;
    virtual bool mounted() const = 0;
    virtual void unmount() = 0;
    virtual bool format() = 0;
    
    virtual std::shared_ptr<File> open(const char* name, FileOpenMode) = 0;
    virtual std::shared_ptr<Directory> openDirectory(const char* name) = 0;
    virtual bool remove(const char* name) = 0;
    virtual bool rename(const char* src, const char* dst) = 0;
    
    virtual uint32_t totalSize() const = 0;
    virtual uint32_t totalUsed() const = 0;
    
    Error lastError() const { return _error; }
    
    static const char* errorString(Error);
    
protected:
    Error _error;
};

class Directory {
    friend class FS;
    
public:
    Directory() { }
    virtual  ~Directory() { }

    const String& name() const { return _name; }
    uint32_t size() const { return _size; }
    bool valid() const { return _error; }
    
    virtual bool next() = 0;
    
protected:
    Error _error;
    String _name;
    uint32_t _size = 0;
};

class File {
    friend class FS;
    
public:
    enum class SeekWhence { Set, Cur, End };
    enum class Type { File, Directory, Unknown };
    
    virtual ~File() { }
  
    virtual int32_t read(char* buf, uint32_t size) = 0;
    virtual int32_t write(const char* buf, uint32_t size) = 0;
    
    int32_t read()
    {
        char buf;
        return (read(&buf, 1) <= 0) ? -1 : static_cast<int>(buf);
    }
    
    int32_t write(uint8_t c)
    {
        return (write(reinterpret_cast<const char*>(&c), 1) <= 0) ? -1 : c;
    }

    virtual  bool seek(int32_t offset, SeekWhence whence) = 0;
    virtual int32_t tell() const = 0;
    virtual bool eof() const = 0;
    
    bool valid() const { return _error; }
    Error error() const { return _error; }
    Type type() const { return _type; }

protected:
    Error _error;
    Type _type = Type::Unknown;
    FS::FileOpenMode _mode = FS::FileOpenMode::Read;
};

}
