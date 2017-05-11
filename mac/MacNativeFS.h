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

#include "FS.h"

#include <cstdio>
#include <dirent.h>
#include <cstring>

namespace m8r {

class MacNativeFS;

class MacNativeDirectoryEntry : public DirectoryEntry {
    friend class MacNativeFS;

public:
    virtual ~MacNativeDirectoryEntry() { if (_dir) ::closedir(_dir); }
    
    virtual bool next() override;
    
private:
    MacNativeDirectoryEntry() { _dir = ::opendir("."); next(); }
    
    DIR* _dir = nullptr;
};

class MacNativeFile : public File {
    friend class MacNativeFS;

public:
    virtual ~MacNativeFile() { if (_file) fclose(_file); }
  
    virtual int32_t read(char* buf, uint32_t size) override { return static_cast<int32_t>(fread(buf, 1, size, _file)); }
    virtual int32_t write(const char* buf, uint32_t size) override { return static_cast<int32_t>(fwrite(buf, 1, size, _file)); }

    virtual bool seek(int32_t offset, File::SeekWhence whence) override
    {
        int origin = (whence == File::SeekWhence::Set) ? SEEK_SET : ((whence == File::SeekWhence::Cur) ? SEEK_CUR : SEEK_END);
        return fseek(_file, offset, origin) == 0;
    }
    
    virtual int32_t tell() const override { return static_cast<int32_t>(ftell(_file)); }
    virtual bool eof() const override { return feof(_file) != 0; }
    
private:
    MacNativeFile(const char* name, const char* mode) { _file = fopen(name, mode); }

    FILE* _file = nullptr;
};

class MacNativeFS : public FS {
public:
    MacNativeFS() { }
    virtual ~MacNativeFS() { }
    
    virtual DirectoryEntry* directory() override { return new MacNativeDirectoryEntry(); }
    virtual bool mount() override { return true; }
    virtual bool mounted() const override { return true; }
    virtual void unmount() override { }
    virtual bool format() override { return true; }
    
    virtual File* open(const char* name, const char* mode) override { return new MacNativeFile(name, mode); }
    virtual bool remove(const char* name) override { return ::remove(name) == 0; }
    virtual bool rename(const char* src, const char* dst) override { return ::rename(src, dst) == 0; }

    virtual uint32_t totalSize() const override { return 0; }
    virtual uint32_t totalUsed() const override { return 0; }

private:
};

}
