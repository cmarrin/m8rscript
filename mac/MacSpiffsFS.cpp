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
    
    addr *= SPIFFS_PHYS_PAGE;
    //size *= SPIFFS_PHYS_PAGE;

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

static s32_t spiffsWrite(u32_t addr, u32_t size, u8_t *src)
{
    if (!fsFile) {
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    
    addr *= SPIFFS_PHYS_PAGE;
    //size *= SPIFFS_PHYS_PAGE;
    
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
    return SPIFFS_OK;
}

void SpiffsFS::setConfig(spiffs_config& config, const char* name)
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
    memset(&config, 0, sizeof(config));
    config.hal_read_f = spiffsRead;
    config.hal_write_f = spiffsWrite;
    config.hal_erase_f = spiffsErase;
}

