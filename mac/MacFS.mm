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

#import <Cocoa/Cocoa.h>

#include "Application.h"
#include "Containers.h"
#include <cstring>
#include <sys/stat.h>
#include <errno.h>

using namespace m8r;

FS* FS::createFS()
{
    return new MacFS();
}

MacFS::MacFS()
{
}

MacFS::~MacFS()
{
}

DirectoryEntry* MacFS::directory()
{
    return new MacDirectoryEntry(_files);
}

bool MacFS::mount()
{
    return _files;
}

bool MacFS::mounted() const
{
    return _files;
}

void MacFS::unmount()
{
    _files = NULL;
}

bool MacFS::format()
{
    if (!_files) {
        return false;
    }
    
    NSArray* filenames = [NSArray arrayWithArray:_files.fileWrappers.allKeys];
    
    for (NSString* name in filenames) {
        [_files removeFileWrapper:_files.fileWrappers[name]];
    }
    return true;
}

File* MacFS::open(const char* name, const char* mode)
{
    return new MacFile(this, name, mode);
}

bool MacFS::remove(const char* name)
{
    if (!mounted()) {
        return false;
    }
    NSFileWrapper* file = _files.fileWrappers[[NSString stringWithUTF8String:name]];
    if (file) {
        [_files removeFileWrapper:file];
    }
    return true;
}

bool MacFS::rename(const char* src, const char* dst)
{
    if (!mounted()) {
        return false;
    }
    NSFileWrapper* file = _files.fileWrappers[[NSString stringWithUTF8String:src]];
    if (file) {
        [_files removeFileWrapper:file];
        file.preferredFilename = [NSString stringWithUTF8String:dst];
        [_files addFileWrapper:file];
        return true;
    }
    return false;
}

uint32_t MacFS::totalSize() const
{
    return MaxSize;
}

uint32_t MacFS::totalUsed() const
{
    // FIXME: Implement
    return MaxSize;
}

MacDirectoryEntry::MacDirectoryEntry(NSFileWrapper* files) : _files(files)
{
	_index = -1;
    next();
}

MacDirectoryEntry::~MacDirectoryEntry()
{
}

bool MacDirectoryEntry::next()
{
    ++_index;
    NSArray* keys = _files.fileWrappers.allKeys;
    if (keys.count <= _index) {
        _valid = false;
        return false;
    }

    strncpy(_name, [keys[_index] UTF8String], FilenameLength - 1);
    _name[FilenameLength - 1] = '\0';
    _size = static_cast<uint32_t>(_files.fileWrappers[keys[_index]].regularFileContents.length);
    _valid = true;
    return true;
}

MacFile::MacFile(MacFS* fs, const char* name, const char* mode) : _files(fs->_files)
{
    if (Application::validateFileName(name) != Application::NameValidationType::Ok) {
        _file = nullptr;
        _error = EBADF;
        return;
    }
    
    if (!fs->mounted()) {
        _file = nullptr;
        _error = ENODEV;
        return;
    }
    
    bool trunc = false;
    bool creat = false;
    bool append = false;
    
    if (strcmp(mode, "r") == 0) {
        _writable = false;
    } else if (strcmp(mode, "w") == 0) {
        _readable = false;
        creat = true;
        trunc = true;
    } else if (strcmp(mode, "w+") == 0) {
        creat = true;
        trunc = true;
    } else if (strcmp(mode, "a") == 0) {
        _readable = false;
        creat = true;
        append = true;
    } else if (strcmp(mode, "a+") == 0) {
        creat = true;
    }
    
    NSString* filename = [NSString stringWithUTF8String:name];
    
    _file = _files.fileWrappers[filename];
    if (_file && trunc) {
        fs->remove(name);
        _file = NULL;
    }
    
    _offset = 0;
    
    if (!_file) {
        if (!creat) {
            _error = _file ? 0 : ENOENT;
        } else {
            [_files addRegularFileWithContents:[[NSData alloc] init] preferredFilename:filename];
            _file = _files.fileWrappers[filename];
        }
    } else if (append) {
        _offset = _file.regularFileContents.length;
    }
}

MacFile::~MacFile()
{
}
  
int32_t MacFile::read(char* buf, uint32_t size)
{
    if (!_file) {
        return -1;
    }
    
    size_t fileSize = _file.regularFileContents.length;
    if (_offset + size > fileSize) {
        size = static_cast<uint32_t>(fileSize - _offset);
    }
    if (size > 0) {
        memcpy(buf, reinterpret_cast<const char*>(_file.regularFileContents.bytes) + _offset, size);
        _offset += size;
    }
    return size;
}

int32_t MacFile::write(const char* buf, uint32_t size)
{
    if (!_file) {
        return -1;
    }
    
    size_t fileSize = _file.regularFileContents.length;
    NSMutableData* data = [NSMutableData dataWithData:_file.regularFileContents];
    
    if (_offset >= fileSize) {
        // append
        [data appendBytes:buf length:size];
    } else if (_offset + size <= fileSize) {
        // overwrite
        [data replaceBytesInRange:NSMakeRange(_offset, size) withBytes:buf];
    } else {
        // overwrite and append
        [data increaseLengthBy:_offset + size - fileSize];
        [data replaceBytesInRange:NSMakeRange(_offset, size) withBytes:buf];
    }
    _offset += size;
    
    NSString* filename = _file.preferredFilename;
    [_files removeFileWrapper:_file];
    [_files addRegularFileWithContents:data preferredFilename:filename];
    _file = _files.fileWrappers[filename];
    return size;
}

bool MacFile::seek(int32_t offset, SeekWhence whence)
{
    if (!_file) {
        return false;
    }
    
    if (whence == SeekWhence::Cur) {
        _offset += offset;
    } else if (whence == SeekWhence::End) {
        _offset = _file.regularFileContents.length - offset;
    } else {
        _offset = offset;
    }
    return true;
}

int32_t MacFile::tell() const
{
    return static_cast<int32_t>(_offset);
}

bool MacFile::eof() const
{
    return !_file || _offset >= _file.regularFileContents.length;
}


