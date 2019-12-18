/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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

static s32_t spiffsWrite(u32_t addr, u32_t size, u8_t *src)
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
    fflush(fsFile);
    return SPIFFS_OK;
}

static s32_t spiffsErase(u32_t addr, u32_t size)
{
    if (fseek(fsFile, addr, SEEK_SET)) {
        printf("******** MacSpiffsFS error seeking to sector %d on erase (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    
    static char buf[SPIFFS_CFG_PHYS_ERASE_SZ()];
    memset(buf, 0xff, size);
    if (fwrite(buf, 1, size, fsFile) != size) {
        printf("******** MacSpiffsFS error erasing sector %d (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return SPIFFS_ERR_NOT_WRITABLE;
    }
    fflush(fsFile);
    return SPIFFS_OK;
}

void SpiffsFS::setConfig(spiffs_config& config)
{
    config.hal_read_f = spiffsRead;
    config.hal_write_f = spiffsWrite;
    config.hal_erase_f = spiffsErase;
}

void SpiffsFS::setHostFilename(const char* name)
{
    if (fsFile) {
        fclose(fsFile);
        fsFile = nullptr;
    }
    
    if (!name) {
        return;
    }
    
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
}
