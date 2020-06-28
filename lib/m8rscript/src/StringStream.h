/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "MStream.h"
#include "Containers.h"

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: StringStream
//
//  This class can take either a String or const char*. If it's a String
//  you can write (append) or read. Otherwise you can just read.
//
//////////////////////////////////////////////////////////////////////////////

class StringStream : public m8r::Stream {
public:
    StringStream() : _s(nullptr), _isString(false) { }
	StringStream(const String& s) : _string(s), _isString(true) { }
	StringStream(const char* s) : _s(s), _isString(false) { }
    
    virtual ~StringStream() { }
	
    bool loaded() { return true; }
    
    virtual int read() const override
    {
        if (_isString) {
            return (_index < _string.size()) ? _string[_index++] : -1;
        } else {
            if (!_s) {
                return -1;
            }
            return (_s[_index] == '\0') ? -1 : _s[_index++];
        }
    }
    
    virtual int write(uint8_t c) override
    {
        if (!_isString) {
            return -1;
        }
        
        // Only allow writing to the end of the string
        if (_index != _string.size()) {
            return -1;
        }
        _string += c;
        _index++;
        return c;
    }
	
private:
    union {
        String _string;
        const char* _s;
    };
    bool _isString = false;
    mutable uint32_t _index = 0;
};

}
