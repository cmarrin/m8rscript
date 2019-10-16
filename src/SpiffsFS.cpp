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

#include "Application.h"

using namespace m8r;

lfs_t SpiffsFS::_spiffsFileSystem;


SpiffsFS::SpiffsFS(const char* name)
{
    memset(&_spiffsFileSystem, 0, sizeof(_spiffsFileSystem));
    setConfig(_config, name);
}

SpiffsFS::~SpiffsFS()
{
    lfs_unmount(&_spiffsFileSystem);
}

bool SpiffsFS::mount()
{
    system()->printf(ROMSTR("Mounting SPIFFS...\n"));
    int32_t result = internalMount();
    if (result != 0) {
        if (result != 0) {
            system()->printf(ROMSTR("ERROR: Not a valid SPIFFS filesystem. Please format.\n"));
            _error = Error::Code::FSNotFormatted;
        } else {
            system()->printf(ROMSTR("ERROR: SPIFFS mount failed, error=%d\n"), result);
            _error = Error::Code::MountFailed;
        }
        return false;
    }
    if (!mounted()) {
        system()->printf(ROMSTR("ERROR: SPIFFS filesystem failed to mount\n"));
        _error = Error::Code::MountFailed;
        return false;
    }

//    system()->printf(ROMSTR("Checking file system...\n"));
//    result = SPIFFS_check(&_spiffsFileSystem);
//    if (result != SPIFFS_OK) {
//        system()->printf(ROMSTR("ERROR: Consistency check failed during SPIFFS mount, error=%d\n"), result);
//        _error = Error::Code::InternalError;
//        return false;
//    }
    
    // Make sure there is a root directory
//    spiffs_file rootDir = SPIFFS_open(SpiffsFS::sharedSpiffs(), "/", SPIFFS_CREAT, 0);
//    if (rootDir < 0) {
//        system()->printf(ROMSTR("ERROR: Could not create root directory, error=%d\n"), rootDir);
//        _error = Error::Code::InternalError;
//        return false;
//    }
//    
//    SPIFFS_close(SpiffsFS::sharedSpiffs(), rootDir);

    system()->printf(ROMSTR("SPIFFS mounted successfully\n"));
    _error = Error::Code::OK;

    return true;
}

bool SpiffsFS::mounted() const
{
    return true; //SPIFFS_mounted(const_cast<spiffs_t*>(&_spiffsFileSystem));
}

void SpiffsFS::unmount()
{
    if (mounted()) {
        lfs_unmount(&_spiffsFileSystem);
    }
}

bool SpiffsFS::format()
{
    if (!mounted()) {
        internalMount();
    }
    unmount();
    
    int32_t result = lfs_format(&_spiffsFileSystem, &_config);
    if (result != 0) {
        system()->printf(ROMSTR("ERROR: SPIFFS format failed, error=%d\n"), result);
        return false;
    }
    mount();
    return true;
}

struct FileModeEntry {
    FS::FileOpenMode _mode;
    int _flags;
};

static const FileModeEntry _fileModeMap[] = {
    { FS::FileOpenMode::Read,  LFS_O_RDONLY },
    { FS::FileOpenMode::ReadUpdate, LFS_O_RDWR },
    { FS::FileOpenMode::Write,  LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC },
    { FS::FileOpenMode::WriteUpdate, LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC },
    { FS::FileOpenMode::Append,  LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND },
    { FS::FileOpenMode::AppendUpdate, LFS_O_RDWR | LFS_O_CREAT },
};

std::shared_ptr<File> SpiffsFS::open(const char* name, FileOpenMode mode)
{
    return std::shared_ptr<File>(new SpiffsFile(name, mode));
}

std::shared_ptr<Directory> SpiffsFS::openDirectory(const char* name, bool create)
{
    if (!mounted()) {
        return nullptr;
    }
    return std::shared_ptr<SpiffsDirectory>(new SpiffsDirectory(name, create));
}

bool SpiffsFS::remove(const char* name)
{
    return lfs_remove(&_spiffsFileSystem, name) == 0;
}

bool SpiffsFS::rename(const char* src, const char* dst)
{
    return lfs_rename(&_spiffsFileSystem, src, dst) == 0;
}

uint32_t SpiffsFS::totalSize() const
{
    return 1000;
}

uint32_t SpiffsFS::totalUsed() const
{
    return 100;
}

int32_t SpiffsFS::internalMount()
{
    return lfs_mount(&_spiffsFileSystem, &_config);
}

//Error::Code SpiffsFS::mapSpiffsError(spiffs_file spiffsError)
//{
//    assert(spiffsError < 0);
//
//    switch(spiffsError) {
//        case SPIFFS_ERR_NOT_MOUNTED         : return Error::Code::NotMounted;
//        case SPIFFS_ERR_FULL                : return Error::Code::NoSpace;
//        case SPIFFS_ERR_NOT_FOUND           : return Error::Code::FileNotFound;
//        case SPIFFS_ERR_END_OF_OBJECT       : return Error::Code::ReadError;
//        case SPIFFS_ERR_OUT_OF_FILE_DESCS   : return Error::Code::TooManyOpenFiles;
//        case SPIFFS_ERR_NOT_WRITABLE        : return Error::Code::NotWritable;
//        case SPIFFS_ERR_NOT_READABLE        : return Error::Code::NotReadable;
//        case SPIFFS_ERR_MOUNTED             : return Error::Code::Mounted;
//        case SPIFFS_ERR_FILE_EXISTS         : return Error::Code::Exists;
//        case SPIFFS_ERR_NOT_CONFIGURED      : return Error::Code::NotMounted;
//        default                             : return Error::Code::InternalError;
//    }
//}

SpiffsDirectory::SpiffsDirectory(const char* name, bool create)
{
    lfs_dir_open(&SpiffsFS::_spiffsFileSystem, &_dir, name);
    next();
}

bool SpiffsDirectory::next()
{
    lfs_info info;
    lfs_dir_read(&SpiffsFS::_spiffsFileSystem, &_dir, &info);
    _size = info.size;
    _name = String(info.name);
    return true;
}

SpiffsFile::SpiffsFile(const char* name, FS::FileOpenMode mode)
{
    // Convert mode to spiffs_flags
    int flags = 0;
    for (int i = 0; i < sizeof(_fileModeMap) / sizeof(FileModeEntry); ++i) {
        if (_fileModeMap[i]._mode == mode) {
            flags = _fileModeMap[i]._flags;
            break;
        }
    }

    lfs_file_open(&SpiffsFS::_spiffsFileSystem, &_file, name, flags);
}

SpiffsFile::~SpiffsFile()
{
    lfs_file_close(&SpiffsFS::_spiffsFileSystem, &_file);
}
  
int32_t SpiffsFile::read(char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Write || _mode == FS::FileOpenMode::Append) {
        _error = Error::Code::NotReadable;
        return -1;
    }
    return lfs_file_read(&SpiffsFS::_spiffsFileSystem, &_file, buf, size);
}

int32_t SpiffsFile::write(const char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Read) {
        _error = Error::Code::NotWritable;
        return -1;
    }
    
    if (_mode == FS::FileOpenMode::AppendUpdate) {
        seek(0, SeekWhence::End);
    }

    return lfs_file_write(&SpiffsFS::_spiffsFileSystem, &_file, const_cast<char*>(buf), size);
}

bool SpiffsFile::seek(int32_t offset, SeekWhence whence)
{
    if (_mode == FS::FileOpenMode::Append) {
        _error = Error::Code::SeekNotAllowed;
        return -1;
    }
    return lfs_file_seek(&SpiffsFS::_spiffsFileSystem, &_file, offset, 0) == 0;
}

int32_t SpiffsFile::tell() const
{
    return lfs_file_tell(&SpiffsFS::_spiffsFileSystem, const_cast<lfs_file_t*>(&_file));
}

bool SpiffsFile::eof() const
{
    return true; //SPIFFS_eof(SpiffsFS::sharedSpiffs(), _file) > 0;
}


