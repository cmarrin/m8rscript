/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MLittleFS.h"

#include "Application.h"

using namespace m8r;

lfs_t LittleFS::_littleFileSystem;


LittleFS::LittleFS()
{
    memset(&_littleFileSystem, 0, sizeof(_littleFileSystem));

    lfs_size_t fsSize = FS::PhysicalSize;

    char* buffer = new char[BufferSize * 3];
    
    memset(&_config, 0, sizeof(_config));
    _config.read_size = 64;
    _config.prog_size = 64;
    _config.block_size =  BlockSize;
    _config.block_count = BlockSize ? fsSize / BlockSize: 0;
    _config.block_cycles = 16; // TODO - need better explanation
    _config.cache_size = 64;
    _config.lookahead_size = 64;
    _config.read_buffer = buffer;
    _config.prog_buffer = buffer + BufferSize;
    _config.lookahead_buffer = buffer + (BufferSize * 2);
    _config.name_max = 0;
    _config.file_max = 0;
    _config.attr_max = 0;
    
    setConfig(_config);
}

LittleFS::~LittleFS()
{
    lfs_unmount(&_littleFileSystem);
}

bool LittleFS::mount()
{
    system()->printf("Mounting LittleFS...\n");
    int32_t result = internalMount();
    if (result != 0) {
        if (result != 0) {
            system()->printf("ERROR: Not a valid LittleFS filesystem. Please format.\n");
            _error = Error::Code::FSNotFormatted;
        } else {
            system()->printf("ERROR: LittleFS mount failed, error=%d\n", result);
            _error = Error::Code::MountFailed;
        }
        return false;
    }
    if (!mounted()) {
        system()->printf("ERROR: LittleFS filesystem failed to mount\n");
        _error = Error::Code::MountFailed;
        return false;
    }

    system()->printf("LittleFS mounted successfully\n");
    _error = Error::Code::OK;

    return true;
}

bool LittleFS::mounted() const
{
    return true; //SPIFFS_mounted(const_cast<spiffs_t*>(&_littleFileSystem));
}

void LittleFS::unmount()
{
    if (mounted()) {
        lfs_unmount(&_littleFileSystem);
    }
}

bool LittleFS::format()
{
    if (!mounted()) {
        internalMount();
    }
    unmount();
    
    int32_t result = lfs_format(&_littleFileSystem, &_config);
    if (result != 0) {
        system()->printf("ERROR: LittleFS format failed, error=%d\n", result);
        return false;
    }
    mount();
    return true;
}

Mad<File> LittleFS::open(const char* name, FileOpenMode mode)
{
    Mad<LittleFile> file = Mad<LittleFile>::create(MemoryType::Native);
    if (_error) {
        file->_error = _error;
    } else {
        file->open(name, mode);
    }
    return file;
}

Mad<Directory> LittleFS::openDirectory(const char* name)
{
    if (!mounted()) {
        return Mad<Directory>();
    }
    return Mad<Directory>();
}

bool LittleFS::makeDirectory(const char* name)
{
    // TODO: For now assume the filename is in the root directory, whether it starts with '/' or not
    _error = Error::Code::OK;
    
    Vector<String> components = String(name).split("/");
    String path = "/";
    for (auto it : components) {
        if (!it.empty()) {
            path += it;
            lfs_error result = static_cast<lfs_error>(lfs_mkdir(&_littleFileSystem, path.c_str()));
            if (result != LFS_ERR_OK && result != LFS_ERR_EXIST) {
                _error = LittleFS::mapLittleError(result);
                return false;
            }
            path += "/";
        }
    }

    return _error == Error::Code::OK;
}

bool LittleFS::remove(const char* name)
{
    return lfs_remove(&_littleFileSystem, name) == 0;
}

bool LittleFS::rename(const char* src, const char* dst)
{
    return lfs_rename(&_littleFileSystem, src, dst) == 0;
}

bool LittleFS::exists(const char* name) const
{
    struct lfs_info info;
    return lfs_stat(&_littleFileSystem, name, &info) == LFS_ERR_OK;
}

uint32_t LittleFS::totalSize() const
{
    return static_cast<uint32_t>(_config.block_count * _config.block_size);
}

uint32_t LittleFS::totalUsed() const
{
    lfs_ssize_t allocatedBlocks = lfs_fs_size(&_littleFileSystem);
    return (allocatedBlocks < 0) ? 0 : (static_cast<uint32_t>(allocatedBlocks) * _config.block_size);
}

int32_t LittleFS::internalMount()
{
    return lfs_mount(&_littleFileSystem, &_config);
}

Error::Code LittleFS::mapLittleError(lfs_error error)
{
    switch(error) {
        case LFS_ERR_OK          : return Error::Code::OK;  // No error
        case LFS_ERR_IO          : return Error::Code::InternalError;  // Error during device operation
        case LFS_ERR_CORRUPT     : return Error::Code::Corrupted;  // Corrupted
        case LFS_ERR_NOENT       : return Error::Code::FileNotFound;  // No directory entry
        case LFS_ERR_EXIST       : return Error::Code::Exists;  // Entry already exists
        case LFS_ERR_NOTDIR      : return Error::Code::NotADirectory;  // Entry is not a dir
        case LFS_ERR_ISDIR       : return Error::Code::NotAFile;  // Entry is a dir
        case LFS_ERR_NOTEMPTY    : return Error::Code::DirectoryNotEmpty;  // Dir is not empty
        case LFS_ERR_BADF        : return Error::Code::InternalError;  // Bad file number
        case LFS_ERR_FBIG        : return Error::Code::NoSpace;  // File too large
        case LFS_ERR_INVAL       : return Error::Code::InternalError;  // Invalid parameter
        case LFS_ERR_NOSPC       : return Error::Code::NoSpace;  // No space left on device
        case LFS_ERR_NOMEM       : return Error::Code::OutOfMemory;  // No more memory available
        case LFS_ERR_NOATTR      : return Error::Code::InternalError;  // No data/attr available
        case LFS_ERR_NAMETOOLONG : return Error::Code::InvalidFileName;  // File name too long
        default                  : return Error::Code::InternalError;
    }
}

LittleDirectory::LittleDirectory(const char* name)
{
    lfs_dir_open(&LittleFS::_littleFileSystem, &_dir, name);
    next();
}

bool LittleDirectory::next()
{
    lfs_info info;
    lfs_dir_read(&LittleFS::_littleFileSystem, &_dir, &info);
    _size = info.size;
    _name = String(info.name);
    return true;
}

static const int _fileModeMap[] = {
    /* FS::FileOpenMode::Read */            LFS_O_RDONLY,
    /* FS::FileOpenMode::ReadUpdate */      LFS_O_RDWR,
    /* FS::FileOpenMode::Write */           LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC,
    /* FS::FileOpenMode::WriteUpdate */     LFS_O_RDWR | LFS_O_CREAT,
    /* FS::FileOpenMode::Append */          LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND,
    /* FS::FileOpenMode::AppendUpdate */    LFS_O_RDWR | LFS_O_CREAT,
    /* FS::FileOpenMode::Create */          LFS_O_RDWR | LFS_O_CREAT,
};

void LittleFile::open(const char* name, FS::FileOpenMode mode)
{
    memset(&_config, 0, sizeof(_config));
    _config.buffer = _buffer;
    lfs_error err = static_cast<lfs_error>(lfs_file_opencfg(&LittleFS::_littleFileSystem, &_file, name, _fileModeMap[static_cast<int>(mode)], &_config));
    _error = LittleFS::mapLittleError(err);
    _mode = mode;
}

LittleFile::~LittleFile()
{
    close();
}
  
int32_t LittleFile::read(char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Write || _mode == FS::FileOpenMode::Append) {
        _error = Error::Code::NotReadable;
        return -1;
    }
    return lfs_file_read(&LittleFS::_littleFileSystem, &_file, buf, size);
}

int32_t LittleFile::write(const char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Read) {
        _error = Error::Code::NotWritable;
        return -1;
    }
    
    if (_mode == FS::FileOpenMode::AppendUpdate) {
        seek(0, SeekWhence::End);
    }

    return lfs_file_write(&LittleFS::_littleFileSystem, &_file, const_cast<char*>(buf), size);
}

void LittleFile::close()
{
    if (valid()) {
        lfs_file_close(&LittleFS::_littleFileSystem, &_file);
    }
    _error = Error::Code::FileClosed;
}

bool LittleFile::seek(int32_t offset, SeekWhence whence)
{
    if (_mode == FS::FileOpenMode::Append) {
        _error = Error::Code::SeekNotAllowed;
        return -1;
    }
    return lfs_file_seek(&LittleFS::_littleFileSystem, &_file, offset, 0) == 0;
}

int32_t LittleFile::tell() const
{
    return lfs_file_tell(&LittleFS::_littleFileSystem, const_cast<lfs_file_t*>(&_file));
}

int32_t LittleFile::size() const
{
    return static_cast<int32_t>(lfs_file_size(&LittleFS::_littleFileSystem, const_cast<lfs_file_t*>(&_file)));
}


