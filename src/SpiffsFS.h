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

#pragma once

#include "FS.h"

#include "Containers.h"
#include "spiffs.h"
#include "spiffs_nucleus.h"

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
//
// Supprted open modes:
//
// Mode           Read/write    File does not exist   File Exists            Seek
// -------------+-------------+---------------------+----------------------+--------------
// Read         | Read only   | NotFound error      | Contents preserved   | Yes
// ReadUpdate   | Read/write  | NotFound error      | Contents preserved   | Yes
// Write        | Write only  | Create file         | Contents deleted     | Yes
// WriteUpdate  | Read/write  | Create file         | Contents deleted     | Yes
// Append       | Write only  | Create file         | Position at end      | No
// AppendUpdate | Read/write  | Create file         | Position at end      | Only for read
//              |             |                     |                      | Resets to end on write
// Create       | Read/write  | Create file         | Contents preserved   | Yes
//
// Note: Create mode is not part of the Posix standard. It gives you the ability to
// open a file for read/write, creates it if it doesn't exist and allows full seek
// with no repositioning on write

namespace m8r {

class SpiffsDirectory : public Directory {
    friend class SpiffsFS;
    
public:
    SpiffsDirectory(const char* name);
    virtual ~SpiffsDirectory() { }
    
    virtual bool next() override;
    
private:
    static constexpr uint8_t FileIDLength = SPIFFS_OBJ_NAME_LEN - 1;

    enum class EntryType { Deleted = 0, Directory = 1, File = 2, Reserved = 3 };
    
    class Entry {
    public:
        EntryType type() const { return static_cast<EntryType>(_value >> 6); }
        uint8_t size() const { return _value & 0x3f; }

    private:
        uint8_t _value;
    };

    class FileID
    {
    public:
        FileID() { }
        
        const char* value() const { return _value; }
        char* value() { return _value; }
        operator bool() const { return _value[0] != '\0'; }
        String str() const { return String(_value, FileIDLength); }
        
        static FileID bad() { return FileID(); }
        static FileID root() { return FileID('/'); }
        static FileID random();

    private:
        FileID(char c) { _value[0] = c; }
        
        char _value[FileIDLength] = { '\0', '\0', '\0' };
    };
    
    static bool find(const char* name, FileID&, File::Type&, Error&);
    static bool findNameInDirectory(const std::shared_ptr<File>&, const String& name, FileID&, File::Type&);

    SpiffsDirectory();
    
    std::shared_ptr<File> _dirFile;
    FileID _fileID;
};

class SpiffsFile : public File {
    friend class SpiffsFS;
    friend class SpiffsDirectory;

public:
    virtual ~SpiffsFile();
  
    virtual int32_t read(char* buf, uint32_t size) override;
    virtual int32_t write(const char* buf, uint32_t size) override;

    virtual bool seek(int32_t offset, File::SeekWhence whence) override;
    virtual int32_t tell() const override;
    virtual bool eof() const override;
    
protected:
    void setType(File::Type type) { _type = type; }
    void setError(Error error) { _error = error; }
    
private:
    SpiffsFile(const char* name, spiffs_flags mode);

    spiffs_file _file = SPIFFS_ERR_FILE_CLOSED;
};

class SpiffsFS : public FS {
    friend SpiffsDirectory;
    friend SpiffsFile;
    
public:
    SpiffsFS(const char* name);
    virtual ~SpiffsFS();
    
    virtual bool mount() override;
    virtual bool mounted() const override;
    virtual void unmount() override;
    virtual bool format() override;
    
    virtual std::shared_ptr<File> open(const char* name, FileOpenMode) override;
    virtual std::shared_ptr<Directory> openDirectory(const char* name) override;
    virtual bool remove(const char* name) override;
    virtual bool rename(const char* src, const char* dst) override;

    virtual uint32_t totalSize() const override;
    virtual uint32_t totalUsed() const override;

private:
    static std::shared_ptr<SpiffsFile> rawOpen(const SpiffsDirectory::FileID&, spiffs_flags, File::Type, FileOpenMode = FileOpenMode::Read);
    
    static Error::Code mapSpiffsError(spiffs_file);

    static void setConfig(spiffs_config&, const char*);
    
    static spiffs* sharedSpiffs()
    {
        return &_spiffsFileSystem;
    }
    
    int32_t internalMount();

    spiffs_config _config;

    static spiffs _spiffsFileSystem;
    uint8_t* _spiffsWorkBuf;
    uint8_t _spiffsFileDescriptors[sizeof(spiffs_fd) * 4];
};

}
