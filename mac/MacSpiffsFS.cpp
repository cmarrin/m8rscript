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

#include "SpiffsFS.h"
#include "spiffs.h"
#include <errno.h>

// The simulated SPIFFS file is a file SPIFFS_PHYS_SIZE in size in the real
// filesystem. The name of that file is passed in

using namespace m8r;

static FILE* fsFile = nullptr;

static s32_t spiffsRead(u32_t addr, u32_t size, u8_t *dst)
{
    if (!fsFile) {
        return SPIFFS_ERR_NOT_READABLE;
    }
    
    if (fseek(fsFile, addr, SEEK_SET)) {
        printf("******** MacSpiffsFS error seeking to sector %d on read (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_READABLE;
    }
    if (fread(dst, 1, size, fsFile) != size) {
        printf("******** MacSpiffsFS error reading sector %d (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_READABLE;
    }
    return SPIFFS_OK;
}

static s32_t spiffsWrite(u32_t addr, u32_t size, const u8_t *src)
{
    if (!fsFile) {
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    
    if (fseek(fsFile, addr, SEEK_SET)) {
        printf("******** MacSpiffsFS error seeking to sector %d on write (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    if (fwrite(src, 1, size, fsFile) != size) {
        printf("******** MacSpiffsFS error writing sector %d (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    return SPIFFS_OK;
}

static s32_t spiffsErase(u32_t addr, u32_t size)
{
    if (fseek(fsFile, addr, SEEK_SET)) {
        printf("******** MacSpiffsFS error seeking to sector %d on erase (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    
    char* buf = new char[size];
    memset(buf, 0xff, size);
    if (fwrite(buf, 1, size, fsFile) != size) {
        printf("******** MacSpiffsFS error erasing sector %d (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    return SPIFFS_OK;
}

static int lfs_flash_read(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, void *dst, lfs_size_t size) {
    uint32_t addr = (block * 256) + off;
    return spiffsRead(addr, size, static_cast<uint8_t*>(dst)) == SPIFFS_OK ? 0 : -1;
}

static int lfs_flash_prog(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = (block * 256) + off;
    const uint8_t *src = reinterpret_cast<const uint8_t *>(buffer);
    return spiffsWrite(addr, size, static_cast<const uint8_t*>(src)) == SPIFFS_OK ? 0 : -1;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = (block * 256);
    uint32_t size = 256;
    return spiffsErase(addr, size) == SPIFFS_OK ? 0 : -1;
}

static int lfs_flash_sync(const struct lfs_config *c) {
    /* NOOP */
    (void) c;
    return 0;
}

void SpiffsFS::setConfig(lfs_config& config, const char* name)
{
    // If the file exists, use it as long as it is big enough
    fsFile = fopen(name, "rb+");
    if (!fsFile) {
        fsFile = fopen(name, "wb+");
        if (!fsFile) {
            printf("******** MacSpiffsFS Error:could not open '%s', file system not available\n", name);
        }
    }
    
    if (fsFile) {
        if (fseek(fsFile, 0, SEEK_END) != 0) {
            printf("******** MacSpiffsFS Error:could not seek to the end of '%s'\n", name);
            fclose(fsFile);
            fsFile = nullptr;
        }
        
        if (ftell(fsFile) < SPIFFS_PHYS_SIZE) {
            ftruncate(fileno(fsFile), SPIFFS_PHYS_SIZE);
        }
    }

    lfs_size_t blockSize = 256;
    lfs_size_t size = 3 * 1024 * 1024;
    
    memset(&config, 0, sizeof(config));
    //config.context = (void*) this;
    config.read = lfs_flash_read;
    config.prog = lfs_flash_prog;
    config.erase = lfs_flash_erase;
    config.sync = lfs_flash_sync;
    config.read_size = 64;
    config.prog_size = 64;
    config.block_size =  blockSize;
    config.block_count = blockSize ? size / blockSize: 0;
    config.block_cycles = 16; // TODO - need better explanation
    config.cache_size = 64;
    config.lookahead_size = 64;
    config.read_buffer = nullptr;
    config.prog_buffer = nullptr;
    config.lookahead_buffer = nullptr;
    config.name_max = 0;
    config.file_max = 0;
    config.attr_max = 0;
}

