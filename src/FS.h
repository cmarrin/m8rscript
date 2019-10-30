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

#include "Containers.h"
#include "Error.h"
#include "Object.h"
#include <cstdint>
#include <memory>

namespace m8r {

class File;
class Directory;

class FS {
    friend class File;
    
public:
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

    enum class FileOpenMode {
        Read            = 0,
        ReadUpdate      = 1,
        Write           = 2,
        WriteUpdate     = 3,
        Append          = 4,
        AppendUpdate    = 5,
        Create          = 6,
    };
    
    FS();
    virtual ~FS() { }

    virtual bool mount() = 0;
    virtual bool mounted() const = 0;
    virtual void unmount() = 0;
    virtual bool format() = 0;
    
    virtual std::shared_ptr<File> open(const char* name, FileOpenMode) = 0;
    virtual std::shared_ptr<Directory> openDirectory(const char* name) = 0;
    virtual bool makeDirectory(const char* name) = 0;
    virtual bool remove(const char* name) = 0;
    virtual bool rename(const char* src, const char* dst) = 0;
    
    virtual uint32_t totalSize() const = 0;
    virtual uint32_t totalUsed() const = 0;
    
    Error lastError() const { return _error; }
    
    static const char* errorString(Error);
    
    // m8rscript object methods
    
protected:
    Error _error;
    
private:
    // m8rscript object methods
    static CallReturnValue mount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue mounted(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static CallReturnValue unmount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue format(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue open(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static CallReturnValue openDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue makeDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue remove(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue rename(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue totalSize(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue totalUsed(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue lastError(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue errorString(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

class Directory {
    friend class FS;
    
public:
    virtual  ~Directory() { }

    const String& name() const { return _name; }
    uint32_t size() const { return _size; }
    bool valid() const { return _error; }
    Error error() const { return _error; }

    virtual bool next() = 0;
    
protected:
    Directory() { }

    Error _error;
    String _name;
    uint32_t _size = 0;

private:
    static CallReturnValue name(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue size(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue valid(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue error(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue next(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

class File {
    friend class FS;
    
public:
    enum class SeekWhence { Set, Cur, End };
    enum class Type { File, Directory, Unknown };
    
    virtual ~File() { }
  
    virtual void close() = 0;
    virtual int32_t read(char* buf, uint32_t size) = 0;
    virtual int32_t write(const char* buf, uint32_t size) = 0;
    
    int32_t read()
    {
        char buf;
        return (read(&buf, 1) <= 0) ? -1 : static_cast<int>(buf);
    }
    
    int32_t write(uint8_t c)
    {
        return (write(reinterpret_cast<const char*>(&c), 1) <= 0) ? -1 : c;
    }

    virtual  bool seek(int32_t offset, SeekWhence whence = SeekWhence::Set) = 0;
    virtual int32_t tell() const = 0;
    virtual int32_t size() const = 0;
    virtual bool eof() const = 0;
    
    bool valid() const { return _error; }
    Error error() const { return _error; }
    Type type() const { return _type; }

protected:
    File() { }

    Error _error = Error::Code::FileClosed;
    Type _type = Type::Unknown;
    FS::FileOpenMode _mode = FS::FileOpenMode::Read;

private:        
    static CallReturnValue close(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue read(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue write(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue seek(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue tell(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue eof(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue valid(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue error(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue type(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
