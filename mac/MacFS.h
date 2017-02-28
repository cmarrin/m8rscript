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

@class NSFileWrapper;

namespace m8r {

class MacDirectoryEntry : public DirectoryEntry {
    friend class MacFS;
    
public:
    virtual ~MacDirectoryEntry();
    
    virtual bool next() override;
    
private:
    MacDirectoryEntry();
    
    int32_t _index;
};

class MacFile : public File {
    friend class MacFS;
    
public:
    virtual ~MacFile();
  
    virtual int32_t read(char* buf, uint32_t size) override;
    virtual int32_t write(const char* buf, uint32_t size) override;

    virtual bool seek(int32_t offset, File::SeekWhence whence) override;
    virtual int32_t tell() const override;
    virtual bool eof() const override;
    
private:
    MacFile(const char* name, const char* mode);

    NSFileWrapper* _file = NULL;
    bool _readable = true;
    bool _writable = true;
    size_t _offset = 0;
};

class MacFS : public FS {
    friend class MacDirectoryEntry;
    friend class MacFile;
    
public:    
    MacFS();
    virtual ~MacFS();
    
    static void setFiles(NSFileWrapper* files) { _files = files; }
    
    virtual DirectoryEntry* directory() override;
    virtual bool mount() override;
    virtual bool mounted() const override;
    virtual void unmount() override;
    virtual bool format() override;
    
    virtual File* open(const char* name, const char* mode) override;
    virtual bool remove(const char* name) override;
    virtual bool rename(const char* src, const char* dst) override;

    virtual uint32_t totalSize() const override;
    virtual uint32_t totalUsed() const override;

private:
    static constexpr uint32_t MaxSize = 4000000;
    
    static NSFileWrapper* _files;
};

}
