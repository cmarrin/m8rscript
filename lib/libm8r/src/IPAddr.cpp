/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "IPAddr.h"

#include "Defines.h"
#include "MString.h"

using namespace m8r;

static bool toIPAddr(const m8r::String& ipString, IPAddr& ip)
{
    Vector<m8r::String> array = ipString.split(".");
    if (array.size() != 4) {
        return false;
    }
    
    for (uint32_t i = 0; i < 4; ++i) {
        uint32_t v;
        if (!m8r::String::toUInt(v, array[i].c_str(), false) || v > 255) {
            return false;
        }
        ip[i] = static_cast<uint8_t>(v);
    }
    return true;
}

IPAddr::IPAddr(const String& ipString)
{
    toIPAddr(ipString, *this);
}

m8r::String IPAddr::toString() const
{
    return String(_addr[0]) + "." +
           String(_addr[1]) + "." +
           String(_addr[2]) + "." +
           String(_addr[3]);
}
