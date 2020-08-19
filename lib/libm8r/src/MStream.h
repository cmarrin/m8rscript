/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cstdint>

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Stream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class Stream {
public:
    virtual int read() const = 0;
    virtual int write(uint8_t) = 0;
	
private:
};

}
