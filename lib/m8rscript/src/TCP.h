/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Object.h"
#include "IPAddr.h"
#include "UDP.h"
#include <cstdint>
#include <cstring>
#include <memory>

namespace m8r {

class TCP;

// Native

class TCP : public NativeObject 
{
    friend class TCPProto;
    
public:
    enum class Event { Connected, Reconnected, Disconnected, ReceivedData, SentData, Error };

    using EventFunction = std::function<void(TCP*, Event, int16_t connectionId, const char* data, int16_t length)>;
    
    static constexpr int MaxConnections = 4;
    static constexpr uint32_t DefaultTimeout = 7200;
     
    virtual ~TCP() { }
    
    void send(int16_t connectionId, const char* data, uint16_t length = 0);
    
    virtual void disconnect(int16_t connectionId) = 0;

protected:
    void init(uint16_t port, IPAddr ip, EventFunction func)
    {
        _eventFunction = func;
        _ip = ip;
        _port = port; 
    }

    virtual int32_t sendData(int16_t connectionId, const char* data, uint16_t length = 0) = 0;

    EventFunction _eventFunction;
    IPAddr _ip;
    uint16_t _port = 0;
    bool _server = false;
};

// Object

class TCPProto : public StaticObject {
public:
    TCPProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue send(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue disconnect(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}
