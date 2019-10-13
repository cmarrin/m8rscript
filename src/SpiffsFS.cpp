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

#include "spiffs_nucleus.h"
#include "Application.h"

using namespace m8r;

spiffs SpiffsFS::_spiffsFileSystem;


SpiffsFS::SpiffsFS(const char* name)
{
    memset(&_spiffsFileSystem, 0, sizeof(_spiffsFileSystem));
    _spiffsWorkBuf = new uint8_t[SPIFFS_CFG_LOG_PAGE_SZ() * 2];
    assert(_spiffsWorkBuf);
    
    setConfig(_config, name);
}

SpiffsFS::~SpiffsFS()
{
    SPIFFS_unmount(&_spiffsFileSystem);
    delete _spiffsWorkBuf;
}

bool SpiffsFS::mount()
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

bool SpiffsFS::mounted() const
{
    return SPIFFS_mounted(const_cast<spiffs_t*>(&_spiffsFileSystem));
}

void SpiffsFS::unmount()
{
    if (mounted()) {
        SPIFFS_unmount(&_spiffsFileSystem);
    }
}

bool SpiffsFS::format()
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
    return true;
}

std::shared_ptr<File> SpiffsFS::find(const char* name)
{
    if (!name || name[0] != '/') {
        return nullptr;
    }
    
    // Split up the name and find each component
    std::shared_ptr<File> file = rawOpen("/", SPIFFS_O_RDONLY);
    std::vector<String> components = String(name).split("/");
    
    for (auto it : components) {
        if (it.empty()) {
            continue;
        }
        
        String fileid = findNameInDirectory(file, it);
        if (fileid.empty()) {
            return nullptr;
        }
        
        file = rawOpen(fileid, SPIFFS_O_RDONLY);
    }
    
    return file;
}

String SpiffsFS::findNameInDirectory(const std::shared_ptr<File>& dir, const String& name)
{
    String s;
    FileID fileId;
    
    while (!dir->eof()) {
        Entry entry;
        dir->read(reinterpret_cast<char*>(&entry), sizeof(entry));
        if (entry.type() == EntryType::File || 
            entry.type() == EntryType::Directory) {
            uint8_t size = entry.size();
            s.reserve(size + 1);
            for ( ; size > 0; --size) {
                char c;
                dir->read(&c, 1);
                s += c;
            }
            
            dir->read(fileId, sizeof(fileId));
            if (s == name) {
                return String(fileId, sizeof(fileId));
            }
        }
    }
    
    return "";
}

void SpiffsFS::createFileID(FileID& fileID)
{
    ::rand();
}

std::shared_ptr<File> SpiffsFS::rawOpen(const String& name, spiffs_flags flags)
{
    return std::shared_ptr<SpiffsFile>(new SpiffsFile(name.c_str(), flags));
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

std::shared_ptr<File> SpiffsFS::open(const char* name, const char* mode)
{
    // Convert mode to spiffs_flags
    spiffs_flags flags = 0;
    for (int i = 0; i < sizeof(_fileModeMap) / sizeof(FileModeEntry); ++i) {
        if (strcmp(mode, _fileModeMap[i]._mode) == 0) {
            flags = _fileModeMap[i]._flags;
            break;
        }
    }
    
    if (!flags) {
        system()->printf(ROMSTR("ERROR: invalid mode '%s' for open\n"), mode);
        return nullptr;
    }
    
    // TODO: If flags allow file creation, generate a new fileId and create the file
    return find(name);
}

std::shared_ptr<Directory> SpiffsFS::openDirectory(const char* name)
{
    if (!mounted()) {
        return nullptr;
    }
    return std::shared_ptr<SpiffsDirectory>(new SpiffsDirectory(name));
}

bool SpiffsFS::remove(const char* name)
{
    return SPIFFS_remove(&_spiffsFileSystem, name) == SPIFFS_OK;
}

bool SpiffsFS::rename(const char* src, const char* dst)
{
    return SPIFFS_rename(&_spiffsFileSystem, src, dst) == SPIFFS_OK;
}

uint32_t SpiffsFS::totalSize() const
{
    u32_t total, used;
    SPIFFS_info(const_cast<spiffs*>(&_spiffsFileSystem), &total, &used);
    return total;
}

uint32_t SpiffsFS::totalUsed() const
{
    u32_t total, used;
    SPIFFS_info(const_cast<spiffs*>(&_spiffsFileSystem), &total, &used);
    return used;
}

int32_t SpiffsFS::internalMount()
{
    return SPIFFS_mount(&_spiffsFileSystem, &_config, _spiffsWorkBuf,
                        _spiffsFileDescriptors, sizeof(_spiffsFileDescriptors), nullptr, 0, NULL);
}

SpiffsDirectory::SpiffsDirectory(const char* name)
{
//    if (
//
//
//
//
//	SPIFFS_opendir(SpiffsFS::sharedSpiffs(), "/", &_dir);
//    next();
}

bool SpiffsDirectory::next()
{
//    spiffs_dirent entry;
//    _valid = SPIFFS_readdir(&_dir, &entry);
//    if (_valid) {
//        strcpy(_name, reinterpret_cast<const char*>(&(entry.name[0])));
//        _size = entry.size;
//    }
    return _valid;
}

SpiffsFile::SpiffsFile(const char* name, spiffs_flags flags)
{
    _file = SPIFFS_open(SpiffsFS::sharedSpiffs(), name, flags, 0);
    _error = (_file < 0) ? static_cast<uint32_t>(-_file) : 0;
}

SpiffsFile::~SpiffsFile()
{
    SPIFFS_close(SpiffsFS::sharedSpiffs(), _file);
}
  
int32_t SpiffsFile::read(char* buf, uint32_t size)
{
    return SPIFFS_read(SpiffsFS::sharedSpiffs(), _file, buf, size);
}

int32_t SpiffsFile::write(const char* buf, uint32_t size)
{
    return SPIFFS_write(SpiffsFS::sharedSpiffs(), _file, const_cast<char*>(buf), size);
}

bool SpiffsFile::seek(int32_t offset, SeekWhence whence)
{
    int whenceFlag = SPIFFS_SEEK_SET;
    if (whence == SeekWhence::Cur) {
        whenceFlag = SPIFFS_SEEK_CUR;
    } else if (whence == SeekWhence::End) {
        whenceFlag = SPIFFS_SEEK_END;
    }
    return SPIFFS_lseek(SpiffsFS::sharedSpiffs(), _file, offset, whenceFlag) == SPIFFS_OK;
}

int32_t SpiffsFile::tell() const
{
    return SPIFFS_tell(SpiffsFS::sharedSpiffs(), _file);
}

bool SpiffsFile::eof() const
{
    return SPIFFS_eof(SpiffsFS::sharedSpiffs(), _file) > 0;
}


