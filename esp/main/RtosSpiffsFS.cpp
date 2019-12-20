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
    return esp_spiffs_mounted(nullptr) == ESP_OK;
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

Mad<SpiffsFile> SpiffsFS::rawOpen(const SpiffsDirectory::FileID& fileID, int flags, File::Type type, FileOpenMode mode)
{
    Mad<SpiffsFile> file = Mad<SpiffsFile>::create(MemoryType::Native);

    file->_file = ::open(fileID.value(), flags);
    file->_error = (file->_file < 0) ? SpiffsFS::mapSpiffsError(errno) : Error::Code::OK;
    
    if (!file->valid()) {
        file->_file = -1;
        return file;
    }
    
    file->setType(type);
    file->_mode = mode;
    return file;
}

Mad<File> SpiffsFS::open(const char* name, FileOpenMode mode)
{
    // If our open mode allows file creation, tell find that it can create a file if it doesn't exist
    SpiffsDirectory::FindCreateMode createMode = SpiffsDirectory::FindCreateMode::File;
    if (mode == FileOpenMode::Read || mode == FileOpenMode::ReadUpdate) {
        createMode = SpiffsDirectory::FindCreateMode::None;
    }

    Mad<SpiffsFile> file;
    SpiffsDirectory::FileID fileID;
    File::Type fileType;
    Error error;
    SpiffsDirectory::find(name, createMode, fileID, fileType, error);

    if (error == Error::Code::OK && fileType != File::Type::File) {
        error = Error::Code::NotAFile;
    } else if (error == Error::Code::OK || error == Error::Code::FileNotFound) {
        // If the file exists its contents are preserved unless it is in Write or
        // WriteAppend mode, in which case it's truncated. If it doesn't exist
        // use the SPIFFS_CREAT flag so the file is created and then opened for
        // read/write
        int flags = O_RDWR;
        
        if (error == Error::Code::OK) {
            if (mode == FileOpenMode::Write || mode == FileOpenMode::WriteUpdate) {
                flags |= O_TRUNC;
            }
        }
        if (mode == FileOpenMode::Read || mode == FileOpenMode::ReadUpdate) {
            error = Error::Code::FileNotFound;
        } else {
            flags |= O_CREAT;
        }
        
        if (fileID) {
            file = SpiffsFS::rawOpen(fileID, flags, File::Type::File, mode);
        }
    }
    
    // At this point file is null because the file doesn't exist and the mode is Read
    // or ReadUpdate. Or it's null because there was an error, which is in 'error'.
    // Or we have a newly created file, an existing file whose contents have been
    // thrown away or an existing file whose contents have been preserved. In all
    // cases we're open read/write
    
    // If file is null, open a dummy file and put 'error' in it.
    if (!file.valid()) {
        assert(error != Error::Code::OK);
        file = SpiffsFS::rawOpen(SpiffsDirectory::FileID::bad(), 0, File::Type::File, mode);
        file->_error = error;
    }
    
    return file;
}

Mad<Directory> SpiffsFS::openDirectory(const char* name)
{
    if (!mounted()) {
        return Mad<Directory>();
    }
    Mad<SpiffsDirectory> dir = Mad<SpiffsDirectory>::create(MemoryType::Native);
    dir->setName(name);
    return dir;
}

bool SpiffsFS::makeDirectory(const char* name)
{
    SpiffsDirectory::FileID fileID;
    File::Type type;
    Error error;
    
    SpiffsDirectory::find(name, SpiffsDirectory::FindCreateMode::Directory, fileID, type, error);
    _error = error;
    return error == Error::Code::OK;
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
    SpiffsDirectory::FileID fileID;
    File::Type fileType;
    Error error;
    SpiffsDirectory::find(name, SpiffsDirectory::FindCreateMode::None, fileID, fileType, error);
    return error == Error::Code::OK;
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
    assert(spiffsError < 0);

    switch(spiffsError) {
        case ESP_OK:                return Error::Code::OK;
        case ESP_ERR_NOT_FOUND:
        case ESP_FAIL:              return Error::Code::InternalError;
        default:                    return Error::Code::InternalError;
    }
}

void SpiffsDirectory::setName(const char* name)
{
    FileID fileID;
    File::Type fileType;
    if (!find(name, SpiffsDirectory::FindCreateMode::None, fileID, fileType, _error)) {
        if (_error == Error::Code::FileNotFound) {
            _error = Error::Code::DirectoryNotFound;
        } else if (fileType != File::Type::Directory) {
            _error = Error::Code::NotADirectory;
        }
        return;
    }
    
    _dirFile = ::open(fileID.value(), O_RDONLY);
    
    if (!_dirFile) {
        _error = Error::Code::InternalError;
        return;
    }

    next();
}

bool SpiffsDirectory::next()
{
    String s;
    
    Entry entry;
    ssize_t size = ::read(_dirFile, reinterpret_cast<char*>(&entry), sizeof(entry));
    
    // TODO: Handle errors
    if (size != sizeof(entry)) {
        return false;
    }
    
    if (entry.type() == EntryType::File ||
        entry.type() == EntryType::Directory) {
        uint8_t size = entry.size();
        s.reserve(size + 1);
        for ( ; size > 0; --size) {
            char c;
            size = ::read(_dirFile, &c, 1);
            if (size != 1) {
                return false;
            }
            s += c;
        }
        
        size = ::read(_dirFile, _fileID.value(), sizeof(_fileID));
        return size == sizeof(_fileID);
    }
    
    return false;
}

bool SpiffsDirectory::find(const char* name, FindCreateMode createMode, FileID& fileID, File::Type& type, Error& error)
{
    error = Error::Code::OK;
    type = File::Type::Unknown;

    if (!name || name[0] != '/') {
        error = Error::Code::InvalidFileName;
        return false;
    }
    
    // Split up the name and find each component
    Mad<SpiffsFile> file(SpiffsFS::rawOpen(FileID::root(), SPIFFS_O_RDWR, File::Type::Directory, FS::FileOpenMode::ReadUpdate));
    if (!file.valid() || !file->valid()) {
        error = Error::Code::InternalError;
    } else {
        Vector<String> components = String(name).split("/");
        
        while (!components.empty() && components.back().empty()) {
            components.pop_back();
        }
        
        error = Error::Code::DirectoryNotFound;

        for (int i = 0; i < components.size(); ++i) {
            if (components[i].empty()) {
                continue;
            }
            
            bool last = i == components.size() - 1;
            
            if (!findNameInDirectory(file.get(), components[i], fileID, type)) {
                if (!last && createMode == FindCreateMode::Directory) {
                    type = File::Type::Directory;
                    createEntry(file.get(), components[i], File::Type::Directory, fileID);
                    file = SpiffsFS::rawOpen(fileID, SPIFFS_O_RDWR | SPIFFS_O_CREAT, File::Type::Directory, FS::FileOpenMode::ReadUpdate);
                    if (!file->valid()) {
                        // We should be able to create the new directory
                        error = Error::Code::InternalError;
                    }
                } else {
                    error = last ? Error::Code::FileNotFound : Error::Code::DirectoryNotFound;
                    
                    // If the error is FileNotFound, the baseName doesn't exist. If we are createMode
                    // is None, just return the error. Otherwise create a file or directory
                    if (error == Error::Code::FileNotFound) {
                        if (createMode == FindCreateMode::None) {
                            break;
                        }
                        
                        type = (createMode == FindCreateMode::File) ? File::Type::File : File::Type::Directory;
                        createEntry(file.get(), components[i], type, fileID);
                        if (type == File::Type::Directory) {
                            file = SpiffsFS::rawOpen(fileID, SPIFFS_O_RDWR | SPIFFS_O_CREAT, type, FS::FileOpenMode::ReadUpdate);
                            if (!file->valid()) {
                                error = Error::Code::InternalError;
                                break;
                            }
                        }
                        error = Error::Code::OK;
                        break;
                    }
                    break;
                }
            } 
            
            if (last) {
                error = Error::Code::OK;
                break;
            }
            
            if (type != File::Type::Directory) {
                error = Error::Code::DirectoryNotFound;
                break;
            }
            
            file.destroy(MemoryType::Native);
            file = SpiffsFS::rawOpen(fileID, SPIFFS_O_RDWR, type, FS::FileOpenMode::ReadUpdate);
            if (!file->valid()) {
                error = Error::Code::InternalError;
                break;
            }
        }
    }
    
    file.destroy(MemoryType::Native);
    if (error != Error::Code::OK) {
        fileID = FileID();
        return false;
    }
    
    return true;
}

bool SpiffsDirectory::findNameInDirectory(File* dir, const String& name, FileID& fileID, File::Type& type)
{
    String s;
    
    while (!dir->eof()) {
        Entry entry;
        dir->read(reinterpret_cast<char*>(&entry), sizeof(entry));
        if (entry.type() == EntryType::File ||
            entry.type() == EntryType::Directory) {
            uint8_t size = entry.size();
            s.clear();
            s.reserve(size + 1);
            for ( ; size > 0; --size) {
                char c;
                dir->read(&c, 1);
                s += c;
            }
            
            FileID testFileID;
            dir->read(testFileID.value(), sizeof(testFileID));
            if (s == name) {
                type = (entry.type() == EntryType::File) ? File::Type::File : File::Type::Directory;
                fileID = testFileID;
                return true;
            }
        }
    }
    
    return false;
}

void SpiffsDirectory::createEntry(File* dir, const String& name, File::Type type, FileID& fileID)
{
    fileID = FileID::random();
    EntryType entryType = (type == File::Type::Directory) ? EntryType::Directory : EntryType::File;
    Entry entry(name.size(), entryType);
    dir->write(entry.value());
    dir->write(name.c_str(), static_cast<uint32_t>(name.size()));
    dir->write(fileID.value(), FileIDLength);
}

SpiffsDirectory::FileID SpiffsDirectory::FileID::random()
{
    SpiffsDirectory::FileID fileID;
    
    char offset = 0x21;
    char range = 0x7e - offset + 1;
    for (uint8_t i = 0; i < FileIDLength; ++i) {
        fileID._value[i] = static_cast<char>(rand() % range) + offset;
    }
    
    return fileID;
}

void SpiffsFile::open(const char* name, spiffs_flags flags)
{
    if (!name || name[0] == '\0') {
        // We're opening a dummy file just so it can hold an error
        return;
    }
    _file = SPIFFS_open(SpiffsFS::sharedSpiffs(), name, flags, 0);
    _error = (_file < 0) ? SpiffsFS::mapSpiffsError(_file) : Error::Code::OK;
}

void SpiffsFile::close()
{
    SPIFFS_close(SpiffsFS::sharedSpiffs(), _file);
    _error = Error::Code::FileClosed;
}

int32_t SpiffsFile::read(char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Write || _mode == FS::FileOpenMode::Append) {
        _error = Error::Code::NotReadable;
        return -1;
    }
    return SPIFFS_read(SpiffsFS::sharedSpiffs(), _file, buf, size);
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

    return SPIFFS_write(SpiffsFS::sharedSpiffs(), _file, const_cast<char*>(buf), size);
}

bool SpiffsFile::seek(int32_t offset, SeekWhence whence)
{
    if (_mode == FS::FileOpenMode::Append) {
        _error = Error::Code::SeekNotAllowed;
        return -1;
    }

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

int32_t SpiffsFile::size() const
{
    spiffs_stat stat;
    s32_t res = SPIFFS_fstat(SpiffsFS::sharedSpiffs(), _file, &stat);
    if (res < SPIFFS_OK) {
        return -1;
    }
    return stat.size;
}

bool SpiffsFile::eof() const
{
    return SPIFFS_eof(SpiffsFS::sharedSpiffs(), _file) > 0;
}


