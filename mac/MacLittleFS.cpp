/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MLittleFS.h"

#include "lfs.h"
#include <errno.h>
#include <unistd.h>

// The simulated LittleFS file is a file FS::PhysicalSize in size in the real
// filesystem. The name of that file is passed in

using namespace m8r;

static FILE* fsFile = nullptr;

static int lfs_flash_read(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, void *dst, lfs_size_t size) {
    uint32_t addr = (block * LittleFS::BlockSize) + off;

    if (!fsFile) {
        return LFS_ERR_INVAL;
    }
    
    if (fseek(fsFile, addr, SEEK_SET)) {
        printf("******** MacLittleFS error seeking to sector %d on read (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return LFS_ERR_INVAL;
    }
    if (fread(dst, 1, size, fsFile) != size) {
        printf("******** MacLittleFS error reading sector %d (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return LFS_ERR_INVAL;
    }
    return LFS_ERR_OK;
}

static int lfs_flash_prog(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = (block * LittleFS::BlockSize) + off;
    const uint8_t *src = reinterpret_cast<const uint8_t *>(buffer);

    if (!fsFile) {
        return LFS_ERR_INVAL;
    }
    
    if (fseek(fsFile, addr, SEEK_SET)) {
        printf("******** MacLittleFS error seeking to sector %d on write (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return LFS_ERR_INVAL;
    }
    if (fwrite(src, 1, size, fsFile) != size) {
        printf("******** MacLittleFS error writing sector %d (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return LFS_ERR_INVAL;
    }
    return LFS_ERR_OK;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = (block * LittleFS::BlockSize);
    uint32_t size = LittleFS::BlockSize;

    if (fseek(fsFile, addr, SEEK_SET)) {
        printf("******** MacLittleFS error seeking to sector %d on erase (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        return LFS_ERR_INVAL;
    }
    
    char* buf = new char[size];
    memset(buf, 0xff, size);
    if (fwrite(buf, 1, size, fsFile) != size) {
        printf("******** MacLittleFS error erasing sector %d (%d): %s\n", addr, ferror(fsFile), strerror(ferror(fsFile)));
        delete [ ] buf;
        return LFS_ERR_INVAL;
    }
    delete [ ] buf;
    return LFS_ERR_OK;
}

static int lfs_flash_sync(const struct lfs_config *c) {
    /* NOOP */
    (void) c;
    return 0;
}

void LittleFS::setHostFilename(const char* name)
{
    // If the file exists, use it as long as it is big enough
    fsFile = fopen(name, "rb+");
    if (!fsFile) {
        fsFile = fopen(name, "wb+");
        if (!fsFile) {
            printf("******** MacLittleFS Error:could not open '%s', file system not available\n", name);
        }
    }
    
    if (fsFile) {
        if (fseek(fsFile, 0, SEEK_END) != 0) {
            printf("******** MacLittleFS Error:could not seek to the end of '%s'\n", name);
            fclose(fsFile);
            fsFile = nullptr;
        }
        
        if (ftell(fsFile) < FS::PhysicalSize) {
            ftruncate(fileno(fsFile), FS::PhysicalSize);
        }
    }
}

void LittleFS::setConfig(lfs_config& config)
{
    config.read = lfs_flash_read;
    config.prog = lfs_flash_prog;
    config.erase = lfs_flash_erase;
    config.sync = lfs_flash_sync;
}
