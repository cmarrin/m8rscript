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

#include "spiffs.h"
#include "spiffs_nucleus.h"

namespace m8r {

class EspDirectoryEntry : public DirectoryEntry {
    friend class EspFS;
    
public:
    virtual ~EspDirectoryEntry();
    
    virtual bool next() override;
    
private:
    EspDirectoryEntry();
    
    spiffs_DIR _dir;
};

class EspFile : public File {
    friend class EspFS;
    
public:
    virtual ~EspFile();
  
    virtual int32_t read(char* buf, uint32_t size) override;
    virtual int32_t write(const char* buf, uint32_t size) override;

    virtual bool seek(int32_t offset, File::SeekWhence whence) override;
    virtual int32_t tell() const override;
    virtual bool eof() const override;
    
private:
    EspFile(const char* name, const char* mode);

    spiffs_file _file = SPIFFS_ERR_FILE_CLOSED;
};

class EspFS : public FS {
    friend EspDirectoryEntry;
    friend EspFile;
    
public:
    EspFS();
    virtual ~EspFS();
    
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
    static int32_t spiffsRead(uint32_t addr, uint32_t size, uint8_t *dst);
    static int32_t spiffsWrite(uint32_t addr, uint32_t size, uint8_t *src);
    static int32_t spiffsErase(uint32_t addr, uint32_t size);

    static spiffs* sharedSpiffs()
    {
        return &(reinterpret_cast<EspFS*>(FS::sharedFS())->_spiffsFileSystem);
    }
    
    int32_t internalMount();

    static FS* _sharedFS;
    
    spiffs_config _config;

    spiffs _spiffsFileSystem;
    uint8_t* _spiffsWorkBuf;
    uint8_t _spiffsFileDescriptors[sizeof(spiffs_fd) * 4];
#if SPIFFS_CACHE
    uint8_t spiffs_cache[(SPIFFS_CFG_LOG_PAGE_SZ() + 32) * 4];
#endif

};

}
