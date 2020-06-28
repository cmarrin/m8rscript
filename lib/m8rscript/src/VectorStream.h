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

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: VectorStream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class VectorStream : public m8r::Stream {
public:
	VectorStream() : _index(0) { }
	VectorStream(const Vector<uint8_t>& vector) : _vector(vector), _index(0) { }
    
    virtual ~VectorStream() { }
	
    bool loaded() { return true; }
    virtual int read() const override
    {
        return (_index < _vector.size()) ? _vector[_index++] : -1;
    }
    virtual int write(uint8_t c) override
    {
        // Only allow writing to the end of the vector
        if (_index != _vector.size()) {
            return -1;
        }
        _vector.push_back(c);
        _index++;
        return c;
    }
    
    void swap(Vector<uint8_t>& vector) { std::swap(vector, _vector); }
	
private:
    Vector<uint8_t> _vector;
    mutable uint32_t _index;
};

}
