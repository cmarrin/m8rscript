/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "RtosSpiffsFS.h"

#include "Application.h"
#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"

#include <errno.h>
#include <fcntl.h>

using namespace m8r;

SpiffsFS::SpiffsFS()
{
}

SpiffsFS::~SpiffsFS()
{
    unmount();
}

Error::Code errorCodeFromErrno(int err)
{
    switch(err) {
        case 0: return Error::Code::OK;
        case EEXIST: return Error::Code::FileExists;
        case ENOENT: return Error::Code::FileNotFound;
        default: return Error::Code::Unknown;
    }
}

bool SpiffsFS::mount()
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    _error = mapSpiffsError(ret);
    return _error;
}

bool SpiffsFS::mounted() const
{
    return esp_spiffs_mounted(nullptr);
}

void SpiffsFS::unmount()
{
    if (mounted()) {
        esp_vfs_spiffs_unregister(nullptr);
    }
}

bool SpiffsFS::format()
{
    esp_err_t err = esp_spiffs_format(nullptr);
    return err == ESP_OK;
}

Mad<File> SpiffsFS::open(const char* name, FileOpenMode mode)
{
    int flags = 0;
    
    switch (mode) {
        default:                            break;
        case FileOpenMode::Read:            flags = O_RDONLY; break;
        case FileOpenMode::ReadUpdate:      flags = O_RDWR; break;
        case FileOpenMode::Write:           flags = O_WRONLY | O_CREAT | O_TRUNC; break;
        case FileOpenMode::WriteUpdate:     flags = O_RDWR | O_CREAT | O_TRUNC; break;
        case FileOpenMode::Append:          flags = O_WRONLY | O_CREAT | O_APPEND; break;
        case FileOpenMode::AppendUpdate:    flags = O_RDWR | O_CREAT | O_APPEND; break;
        case FileOpenMode::Create:          flags = O_RDWR | O_CREAT; break;
    }
    
    Mad<SpiffsFile> file = Mad<SpiffsFile>::create(MemoryType::Native);
    String filename = "/spiffs";
    filename += name;
    file->_file = ::open(filename.c_str(), flags);
    file->_error = errorCodeFromErrno(errno);
    return file;
}

Mad<Directory> SpiffsFS::openDirectory(const char* name)
{
    if (!mounted()) {
        return Mad<Directory>();
    }
    Mad<SpiffsDirectory> dir = Mad<SpiffsDirectory>::create(MemoryType::Native);
    return dir;
}

bool SpiffsFS::makeDirectory(const char* name)
{
    // TODO: Implement
    return false;
}

bool SpiffsFS::remove(const char* name)
{
    // TODO: Implement
    return false;
}

bool SpiffsFS::rename(const char* src, const char* dst)
{
    // TODO: Implement
    return false;
}

bool SpiffsFS::exists(const char* name) const
{
    Mad<File> file = const_cast<SpiffsFS*>(this)->open(name, FileOpenMode::Read);
    bool doesExist = file->error() == Error::Code::OK;
    file->close();
    return doesExist;
}

uint32_t SpiffsFS::totalSize() const
{
    size_t total, used;
    esp_spiffs_info(nullptr, &total, &used);
    return total;
}

uint32_t SpiffsFS::totalUsed() const
{
    size_t total, used;
    esp_spiffs_info(nullptr, &total, &used);
    return used;
}

Error::Code SpiffsFS::mapSpiffsError(int spiffsError)
{
    switch(spiffsError) {
        case ESP_OK:                return Error::Code::OK;
        case ESP_ERR_NOT_FOUND:
        case ESP_FAIL:              return Error::Code::InternalError;
        default:                    return Error::Code::InternalError;
    }
}

bool SpiffsDirectory::next()
{
    struct dirent* entry = readdir(_dir);
    if (!entry) {
        return false;
    }
    _name = String(entry->d_name);
    
    // TODO: Get file size from stat
    _size = 0;
    return true;
}

void SpiffsFile::close()
{
    ::close(_file);
    _error = Error::Code::FileClosed;
}

int32_t SpiffsFile::read(char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Write || _mode == FS::FileOpenMode::Append) {
        _error = Error::Code::NotReadable;
        return -1;
    }
    return ::read(_file, buf, size);
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

    return ::write(_file, const_cast<char*>(buf), size);
}

bool SpiffsFile::seek(int32_t offset, SeekWhence whence)
{
    if (_mode == FS::FileOpenMode::Append) {
        _error = Error::Code::SeekNotAllowed;
        return -1;
    }

    int whenceFlag = SEEK_SET;
    if (whence == SeekWhence::Cur) {
        whenceFlag = SEEK_CUR;
    } else if (whence == SeekWhence::End) {
        whenceFlag = SEEK_END;
    }
    return lseek(_file, offset, whenceFlag) >= 0;
}

int32_t SpiffsFile::tell() const
{
    return lseek(_file, SEEK_CUR, 0);
}

int32_t SpiffsFile::size() const
{
    struct stat stat;
    int res = fstat(_file, &stat);
    if (res < 0) {
        return -1;
    }
    return stat.st_size;
}
