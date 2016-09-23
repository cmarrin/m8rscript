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

#include "MacFS.h"

#include "Containers.h"
#include <cstring>
#include <sys/stat.h>
#include <errno.h>

using namespace m8r;

FS* FS::_sharedFS = nullptr;
char* MacFS::_basePath = nullptr;

void MacFS::setFileSystemPath(const char* path)
{
    if (_basePath) {
        free(_basePath);
        _basePath = nullptr;
    }
    
    if (!path) {
        return;
    }
    
    size_t size = strlen(path);
    if (path[size - 1] != '/') {
        _basePath = static_cast<char*>(malloc(size + 2));
        memcpy(_basePath, path, size);
        _basePath[size] = '/';
        _basePath[size + 1] = '\0';
    } else {
        _basePath = strdup(path);
    }
}

FS* FS::sharedFS()
{
    if (!_sharedFS) {
        _sharedFS = new MacFS();
    }
    return _sharedFS;
}

MacFS::MacFS()
{
}

MacFS::~MacFS()
{
}

DirectoryEntry* MacFS::directory()
{
    return new MacDirectoryEntry();
}

bool MacFS::mount()
{
    return true;
}

bool MacFS::mounted() const
{
    return _basePath;
}

void MacFS::unmount()
{
}

bool MacFS::format()
{
    if (!_basePath) {
        return false;
    }
    DirectoryEntry* entry = directory();
    while (entry && entry->valid()) {
        String s = String(_basePath) + entry->name();
        remove(s.c_str());
    }
    return true;
}

File* MacFS::open(const char* name, const char* mode)
{
    return new MacFile(name, mode);
}

bool MacFS::remove(const char* name)
{
    if (!_basePath) {
        return false;
    }
    String path = String(_basePath) + name;
    return ::remove(path.c_str()) == 0;
}

MacDirectoryEntry::MacDirectoryEntry()
{
	_dir = MacFS::_basePath ? ::opendir(MacFS::_basePath) : nullptr;
    next();
}

MacDirectoryEntry::~MacDirectoryEntry()
{
    if (_dir) {
        ::closedir(_dir);
    }
}

bool MacDirectoryEntry::next()
{
    if (!_dir) {
        return false;
    }
    
    struct dirent* entry = ::readdir(_dir);
    if (!entry) {
        _valid = false;
        return false;
    }
    
    if (entry->d_type != DT_REG || entry->d_name[0] == '.') {
        return next();
    }
    
    strncpy(_name, entry->d_name, FilenameLength - 1);
    String path = String(MacFS::_basePath) + "/" + _name;
    struct stat statEntry;
    if (::stat(path.c_str(), &statEntry) != 0) {
        _valid = false;
        return false;
    }
    _size = static_cast<uint32_t>(statEntry.st_size);
    _valid = true;
    return true;
}

MacFile::MacFile(const char* name, const char* mode)
{
    if (!MacFS::_basePath) {
        _file = nullptr;
        _error = ENODEV;
        return;
    }
    String path = String(MacFS::_basePath) + name;
    _file = ::fopen(path.c_str(), mode);
    _error = _file ? 0 : errno;
}

MacFile::~MacFile()
{
    if (_file) {
        ::fclose(_file);
    }
}
  
int32_t MacFile::read(char* buf, uint32_t size)
{
    return _file ? static_cast<int32_t>(::fread(buf, 1, size, _file)) : -1;
}

int32_t MacFile::write(const char* buf, uint32_t size)
{
    return _file ? static_cast<int32_t>(::fwrite(buf, 1, size, _file)) : -1;
}

bool MacFile::seek(int32_t offset, SeekWhence whence)
{
    if (!_file) {
        return false;
    }
    
    int whenceFlag = SEEK_SET;
    if (whence == SeekWhence::Cur) {
        whenceFlag = SEEK_CUR;
    } else if (whence == SeekWhence::End) {
        whenceFlag = SEEK_END;
    }
    return ::fseek(_file, offset, whenceFlag) == 0;
}

int32_t MacFile::tell() const
{
    return _file ? static_cast<int32_t>(::ftell(_file)) : -1;
}

bool MacFile::eof() const
{
    return !_file || ::feof(_file) != 0;
}


