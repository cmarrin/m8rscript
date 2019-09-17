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

#include "EspFS.h"

#include "Application.h"
#include "Esp.h"

using namespace m8r;

spiffs EspFS::_spiffsFileSystem;


EspFS::EspFS()
{
    memset(&_spiffsFileSystem, 0, sizeof(_spiffsFileSystem));
    _spiffsWorkBuf = new uint8_t[SPIFFS_CFG_LOG_PAGE_SZ() * 2];
    assert(_spiffsWorkBuf);

    memset(&_config, 0, sizeof(_config));
    _config.hal_read_f = spiffsRead;
    _config.hal_write_f = spiffsWrite;
    _config.hal_erase_f = spiffsErase;
}

EspFS::~EspFS()
{
    SPIFFS_unmount(&_spiffsFileSystem);
    delete _spiffsWorkBuf;
}

DirectoryEntry* EspFS::directory()
{
    if (!mounted()) {
        return nullptr;
    }
    return new EspDirectoryEntry();
}

bool EspFS::mount()
{
    system()->printf(ROMSTR("Mounting SPIFFS...\n"));
    int32_t result = internalMount();
    if (result != SPIFFS_OK) {
        if (result == SPIFFS_ERR_NOT_A_FS) {
            system()->printf(ROMSTR("ERROR: Not a valid SPIFFS filesystem. Please format.\n"));
        } else {
            system()->printf(ROMSTR("ERROR: SPIFFS mount failed, error=%d\n"), result);
        }
        return false;
    }
    if (!mounted()) {
        system()->printf(ROMSTR("ERROR: SPIFFS filesystem failed to mount\n"));
        return false;
    }

    system()->printf(ROMSTR("Checking file system...\n"));
    result = SPIFFS_check(&_spiffsFileSystem);
    if (result != SPIFFS_OK) {
        system()->printf(ROMSTR("ERROR: Consistency check failed during SPIFFS mount, error=%d\n"), result);
        return false;
    } else {
        system()->printf(ROMSTR("SPIFFS mounted successfully\n"));
    }
    return true;
}

bool EspFS::mounted() const
{
    return SPIFFS_mounted(const_cast<spiffs_t*>(&_spiffsFileSystem));
}

void EspFS::unmount()
{
    if (mounted()) {
        SPIFFS_unmount(&_spiffsFileSystem);
    }
}

bool EspFS::format()
{
    if (!mounted()) {
        internalMount();
    }
    unmount();
    
    int32_t result = SPIFFS_format(&_spiffsFileSystem);
    if (result != SPIFFS_OK) {
        system()->printf(ROMSTR("ERROR: SPIFFS format failed, error=%d\n"), result);
        return false;
    }
    mount();
    writeUserData();
    return true;
}

File* EspFS::open(const char* name, const char* mode)
{
    return new EspFile(name, mode);
}

bool EspFS::remove(const char* name)
{
    return SPIFFS_remove(&_spiffsFileSystem, name) == SPIFFS_OK;
}

bool EspFS::rename(const char* src, const char* dst)
{
    return SPIFFS_rename(&_spiffsFileSystem, src, dst) == SPIFFS_OK;
}

uint32_t EspFS::totalSize() const
{
    u32_t total, used;
    SPIFFS_info(const_cast<spiffs*>(&_spiffsFileSystem), &total, &used);
    return total;
}

uint32_t EspFS::totalUsed() const
{
    u32_t total, used;
    SPIFFS_info(const_cast<spiffs*>(&_spiffsFileSystem), &total, &used);
    return used;
}

int32_t EspFS::internalMount()
{
    return SPIFFS_mount(&_spiffsFileSystem, &_config, _spiffsWorkBuf,
                        _spiffsFileDescriptors, sizeof(_spiffsFileDescriptors), nullptr, 0, NULL);
}

s32_t EspFS::spiffsRead(u32_t addr, u32_t size, u8_t *dst)
{
    return (flashmem_read(dst, addr, size) == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_READABLE;
}

s32_t EspFS::spiffsWrite(u32_t addr, u32_t size, u8_t *src)
{
    return (flashmem_write(src, addr, size) == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_WRITABLE;
}

s32_t EspFS::spiffsErase(u32_t addr, u32_t size)
{
    u32_t firstSector = flashmem_get_sector_of_address(addr);
    u32_t lastSector = firstSector;
    while(firstSector <= lastSector) {
        if(!flashmem_erase_sector(firstSector++)) {
            return SPIFFS_ERR_INTERNAL;
        }
    }
    return SPIFFS_OK;
}

EspDirectoryEntry::EspDirectoryEntry()
{
	SPIFFS_opendir(EspFS::sharedSpiffs(), "/", &_dir);
    next();
}

EspDirectoryEntry::~EspDirectoryEntry()
{
    SPIFFS_closedir(&_dir);
}

bool EspDirectoryEntry::next()
{
    spiffs_dirent entry;
    _valid = SPIFFS_readdir(&_dir, &entry);
    if (_valid) {
        strcpy(_name, reinterpret_cast<const char*>(&(entry.name[0])));
        _size = entry.size;
    }
    return _valid;
}

struct FileModeEntry {
    const char* _mode;
    spiffs_flags _flags;
};

static const FileModeEntry _fileModeMap[] = {
    { "r",  SPIFFS_RDONLY },
    { "r+", SPIFFS_RDWR },
    { "w",  SPIFFS_WRONLY | SPIFFS_CREAT | SPIFFS_TRUNC },
    { "w+", SPIFFS_RDWR | SPIFFS_CREAT | SPIFFS_TRUNC },
    { "a",  SPIFFS_WRONLY | SPIFFS_CREAT | SPIFFS_APPEND },
    { "a+", SPIFFS_RDWR | SPIFFS_CREAT },
};

EspFile::EspFile(const char* name, const char* mode)
{
    if (Application::validateFileName(name) != Application::NameValidationType::Ok) {
        system()->printf(ROMSTR("ERROR: invalid filename '%s' for open\n"), name);
        _file = SPIFFS_ERR_NAME_TOO_LONG;
        return;
    }
    
    spiffs_flags flags = 0;
    for (int i = 0; i < sizeof(_fileModeMap) / sizeof(FileModeEntry); ++i) {
        if (strcmp(mode, _fileModeMap[i]._mode) == 0) {
            flags = _fileModeMap[i]._flags;
            break;
        }
    }
    if (!flags) {
        system()->printf(ROMSTR("ERROR: invalid mode '%s' for open\n"), mode);
        _file = SPIFFS_ERR_FILE_CLOSED;
        return;
    }
    _file = SPIFFS_open(EspFS::sharedSpiffs(), name, flags, 0);
    _error = (_file < 0) ? static_cast<uint32_t>(-_file) : 0;
}

EspFile::~EspFile()
{
    SPIFFS_close(EspFS::sharedSpiffs(), _file);
}
  
int32_t EspFile::read(char* buf, uint32_t size)
{
    return SPIFFS_read(EspFS::sharedSpiffs(), _file, buf, size);
}

int32_t EspFile::write(const char* buf, uint32_t size)
{
    int32_t count =  SPIFFS_write(EspFS::sharedSpiffs(), _file, const_cast<char*>(buf), size);
}

bool EspFile::seek(int32_t offset, SeekWhence whence)
{
    int whenceFlag = SPIFFS_SEEK_SET;
    if (whence == SeekWhence::Cur) {
        whenceFlag = SPIFFS_SEEK_CUR;
    } else if (whence == SeekWhence::End) {
        whenceFlag = SPIFFS_SEEK_END;
    }
    return SPIFFS_lseek(EspFS::sharedSpiffs(), _file, offset, whenceFlag) == SPIFFS_OK;
}

int32_t EspFile::tell() const
{
    return SPIFFS_tell(EspFS::sharedSpiffs(), _file);
}

bool EspFile::eof() const
{
    return SPIFFS_eof(EspFS::sharedSpiffs(), _file) > 0;
}


