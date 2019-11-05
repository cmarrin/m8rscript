/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "FS.h"

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
    virtual bool eof() const override;
    
protected:
    void setType(File::Type type) { _type = type; }
    void setError(Error error) { _error = error; }
    
private:
    LittleFile(const char* name, FS::FileOpenMode mode);

    lfs_file_t _file;
};

class LittleFS : public FS {
    friend LittleDirectory;
    friend LittleFile;
    
public:
    LittleFS(const char* name);
    virtual ~LittleFS();
    
    virtual bool mount() override;
    virtual bool mounted() const override;
    virtual void unmount() override;
    virtual bool format() override;
    
    virtual File* open(const char* name, FileOpenMode) override;
    virtual std::shared_ptr<Directory> openDirectory(const char* name) override;
    virtual bool makeDirectory(const char* name) override;
    virtual bool remove(const char* name) override;
    virtual bool rename(const char* src, const char* dst) override;

    virtual uint32_t totalSize() const override;
    virtual uint32_t totalUsed() const override;

private:
    static Error::Code mapLittleError(lfs_error);

    static void setConfig(lfs_config&, const char*);
    
    static lfs_t* sharedLittle()
    {
        return &_littleFileSystem;
    }
    
    int32_t internalMount();

    lfs_config _config;

    static lfs_t _littleFileSystem;
};

}
