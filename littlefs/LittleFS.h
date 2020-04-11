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

#ifdef __cplusplus
extern "C" {
#endif
#include "lfs.h"
#ifdef __cplusplus
} // extern "C"
#endif

// LittleFS File System

namespace m8r {

class LittleFS : public FS {
    friend class LittleDirectory;
    friend class LittleFile;
    
public:
    static constexpr uint32_t BufferSize = 64;
    static constexpr uint32_t BlockSize = 8 * 1024;

    LittleFS();
    virtual ~LittleFS();
    
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
    static Error::Code mapLittleError(lfs_error);

    static void setConfig(lfs_config&);
    
    static lfs_t* sharedLittle()
    {
        return &_littleFileSystem;
    }
    
    int32_t internalMount();

    lfs_config _config;

    static lfs_t _littleFileSystem;
};

class LittleDirectory : public Directory {
    friend class LittleFS;
    
public:
    virtual ~LittleDirectory() { }
    
    virtual bool next() override;
    
private:
    LittleDirectory();
    LittleDirectory(const char* name);

    lfs_dir_t _dir;
};

class LittleFile : public File {
    friend class LittleFS;
    friend class LittleDirectory;

public:
    virtual ~LittleFile();
  
    virtual int32_t read(char* buf, uint32_t size) override;
    virtual int32_t write(const char* buf, uint32_t size) override;
    virtual void close() override;

    virtual bool seek(int32_t offset, File::SeekWhence whence) override;
    virtual int32_t tell() const override;
    virtual int32_t size() const override;
    
protected:
    void setType(File::Type type) { _type = type; }
    void setError(Error error) { _error = error; }
    
private:
    void open(const char* name, FS::FileOpenMode);

    lfs_file_t _file;
    struct lfs_file_config _config;
    char _buffer[LittleFS::BufferSize];
};

}
