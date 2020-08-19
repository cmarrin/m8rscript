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
#include "MStream.h"

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: FileStream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class FileStream : public Stream {
public:
	FileStream(Mad<File> file)
        : _file(file)
    { }
    
    ~FileStream() { }

    bool loaded()
    {
        return _file.valid() && _file->valid();
    }
    virtual int read() const override
    {
        if (!_file.valid()) {
            return -1;
        }
        char c;
        if (_file->read(&c, 1) != 1) {
            return -1;
        }
        return static_cast<uint8_t>(c);
    }
    virtual int write(uint8_t c) override
    {
        if (!_file.valid()) {
            return -1;
        }
        if (_file->write(reinterpret_cast<char*>(&c), 1) != 1) {
            return -1;
        }
        return c;
    }
	
private:
    Mad<File> _file;
};

}
