/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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

class FileProto : public StaticObject {
public:
    FileProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
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

class DirectoryProto : public StaticObject {
public:
    DirectoryProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue name(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue size(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue type(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue valid(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue error(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue next(ExecutionUnit*, Value thisValue, uint32_t nparams);
};    

class FSProto : public StaticObject {
public:
    FSProto();
        
    static CallReturnValue mount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue mounted(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static CallReturnValue unmount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue format(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue open(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static CallReturnValue openDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue makeDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue remove(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue rename(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue stat(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue lastError(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue errorString(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

class FS {
    friend class File;
    
public:
    static constexpr uint32_t PhysicalSize = 3 * 1024 * 1024;
    
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
    
    FS() { }
    virtual ~FS() { }

    virtual bool mount() = 0;
    virtual bool mounted() const = 0;
    virtual void unmount() = 0;
    virtual bool format() = 0;
    
    virtual Mad<File> open(const char* name, FileOpenMode) = 0;
    virtual Mad<Directory> openDirectory(const char* name) = 0;
    virtual bool makeDirectory(const char* name) = 0;
    virtual bool remove(const char* name) = 0;
    virtual bool rename(const char* src, const char* dst) = 0;
    virtual bool exists(const char* name) const = 0;
    
    virtual uint32_t totalSize() const = 0;
    virtual uint32_t totalUsed() const = 0;
    
    Error lastError() const { return _error; }
    
    static const char* errorString(Error);
    
    static String findPath(ExecutionUnit*, const String& filename, const Mad<Object>& env);
    
    // m8rscript object methods
    
protected:
    Error _error;
};

class Directory : public NativeObject {
    friend class FS;
    
public:
    virtual  ~Directory() { }

    const String& name() const { return _name; }
    uint32_t size() const { return _size; }
    bool valid() const { return _error == Error::Code::OK; }
    Error error() const { return _error; }

    virtual bool next() = 0;
    
protected:
    Directory() { }

    Error _error;
    String _name;
    uint32_t _size = 0;
};

class File : public NativeObject {
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
    
    bool valid() const { return _error == Error::Code::OK; }
    Error error() const { return _error; }
    Type type() const { return _type; }

protected:
    File() { }

    Error _error = Error::Code::FileClosed;
    Type _type = Type::Unknown;
    FS::FileOpenMode _mode = FS::FileOpenMode::Read;
};

}
