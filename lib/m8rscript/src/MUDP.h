/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "IPAddr.h"
#include "Object.h"
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <memory>

namespace m8r {

class UDP;
class IPAddr;

// Native

class UDP : public NativeObject {
public:
    enum class Event { Disconnected, ReceivedData, SentData, Error };

    using EventFunction = std::function<void(UDP*, Event, const char* data, int16_t length)>;
    
    virtual ~UDP() { }
        
    static void joinMulticastGroup(IPAddr);
    static void leaveMulticastGroup(IPAddr);
    
    virtual void send(IPAddr, uint16_t port, const char* data, uint16_t length = 0) = 0;
    virtual void disconnect() = 0;

protected:
    void init(uint16_t port, EventFunction func)
    {
        _eventFunction = func;
        _port = port;  
    }

    EventFunction _eventFunction;
    uint16_t _port;
};

// Object

#if M8RSCRIPT_SUPPORT == 1
class UDPProto : public StaticObject {
public:
    UDPProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue send(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue disconnect(ExecutionUnit*, Value thisValue, uint32_t nparams);
};
#endif

}
