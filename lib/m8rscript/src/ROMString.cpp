/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "ROMString.h"

#include "MString.h"
#include <memory>

using namespace m8r;

m8r::String ROMString::copy() const
{
    return String(*this);
}

m8r::String ROMString::vformat(ROMString format, va_list args)
{
    va_list args2;
    va_copy(args2, args);
    String f = format.copy();
    size_t size = ::vsnprintf(nullptr, 0, f.c_str(), args) + 1;
    if( size <= 0 ) {
        return "***** Error during formatting.";
    }
    std::unique_ptr<char[]> buf(new char[size]); 
    ::vsnprintf(buf.get(), size, f.c_str(), args2);
    return String(buf.get(), int32_t(size - 1));
}

m8r::String ROMString::format(ROMString format, ...)
{
    va_list args;
    va_start(args, format);
    return vformat(format, args);
}
