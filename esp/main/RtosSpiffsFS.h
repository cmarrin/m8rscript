/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "MFS.h"

#include "Containers.h"

#include <dirent.h>

// Spiffs++ File System
//
// The underlying file system is Spiffs. But that doesn't provide a
// hierarchical directory structure. So Spiffs++ layers that on top.
// The root system directory is at "/". This contains a directory file
// with sequential entries. Each entry has a filename and information
// about its type and location in the Spiffs file system. The entry
// contains a file id, which is a 3 character filename in the actual
// Spiffs file system. This name is passed to SPIFFS_open() to open the 
// actual file.
//
// Generating file ids
//
// When a file is created a random file id is generated. A SPIFFS_open() is 
// then done with the SPIFFS_RDONLY flag. If the file opens successfully
// it means a file with that name already exists so we generate another
// random name and try again. The name range is 255^3 or 16 million
// so the chance of collision is incredibly rare.
//
// Directory structure
//
// A directory is just a file containing a sequence of entries. Each entry
// starts with a byte in which the lower 6 bits are the length of the
// name, allowing for 64 character names max. The upper 2 bits 
// indicate the file type. Currently 00b is a deleted file, 01b is a 
// directory, 10b is a file and 11b is reserved for future use. Following
// this byte is the filename itself, occupying the number of bytes 
// indicated in the length field of the first byte. After the name is
// a 3 character filename in the actual Spiffs file system.
//
// Deleted file entries, indicated by a type value of 00b, can be used
// if the new file name is <= the old name. If the new name is shorter,
// the space is filled with '\0'.
//
// When placing a new name is a directory, it is searched from the 
// beginning. If no deleted entries are found that fit, the name is 
// appended to the directory file.
//
// TODO: compress the deleted file space when it gets too large

namespace m8r {

class SpiffsDirectory : public Directory {
    friend class SpiffsFS;
    
public:
    SpiffsDirectory() { _dir = opendir("/spiffs"); }
    virtual ~SpiffsDirectory() { closedir(_dir); }
    
    virtual bool next() override;

private:
    DIR* _dir;
};

class SpiffsFile : public File {
    friend class SpiffsFS;
    friend class SpiffsDirectory;

public:
    SpiffsFile() { }
    virtual ~SpiffsFile() { close(); }
  
    virtual void close() override;
    virtual int32_t read(char* buf, uint32_t size) override;
    virtual int32_t write(const char* buf, uint32_t size) override;

    virtual bool seek(int32_t offset, File::SeekWhence whence) override;
    virtual int32_t tell() const override;
    virtual int32_t size() const override;
    
protected:
    void setType(File::Type type) { _type = type; }
    void setError(Error error) { _error = error; }
    
private:
    int _file = -1;
};

class SpiffsFS : public FS {
    friend SpiffsDirectory;
    friend SpiffsFile;
    
public:
    SpiffsFS();
    virtual ~SpiffsFS();
    
    virtual bool mount() override;
    virtual bool mounted() const override;
    virtual void unmount() override;
    virtual bool format() override;
    
    virtual Mad<File> open(const char* name, FileOpenMode) override;
    virtual Mad<Directory> openDirectory(const char* name) override;
    virtual bool makeDirectory(const char* name) override;
    virtual bool remove(const char* name) override;
    virtual bool rename(const char* src, const char* dst) override;
    virtual bool exists(const char* name) const override;

    virtual uint32_t totalSize() const override;
    virtual uint32_t totalUsed() const override;

    static void setHostFilename(const char*);

private:
    static constexpr uint32_t MaxOpenFiles = 4;

    static Error::Code mapSpiffsError(int);
};

}
