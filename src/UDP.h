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

class UDPDelegate {
public:
    enum class Event { Disconnected, ReceivedData, SentData, Error };
    
    virtual void UDPevent(UDP*, Event, const char* data = nullptr, uint16_t length = 0) = 0;
};

class UDP {
public:
    virtual ~UDP() { }
        
    static void joinMulticastGroup(IPAddr);
    static void leaveMulticastGroup(IPAddr);
    
    virtual void send(IPAddr, uint16_t port, const char* data, uint16_t length = 0) = 0;
    virtual void disconnect() = 0;

protected:
    void init(UDPDelegate* delegate, uint16_t port)
    {
        _delegate = delegate;
        _port = port;  
    }

    UDPDelegate* _delegate;
    uint16_t _port;
};

// Object

class UDPProto : public StaticObject {
public:
    UDPProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue send(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue disconnect(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

class MyUDPDelegate : public NativeObject, public UDPDelegate {
public:
    MyUDPDelegate() { }
    virtual ~MyUDPDelegate() { }

    void init(ExecutionUnit*, uint16_t port, const Value& func, const Value& parent);

    void send(IPAddr ip, uint16_t port, const char* data, uint16_t size)
    {
        if (!_udp.valid()) {
            return;
        }
        _udp->send(ip, port, data, size);
    }
    
    void disconnect()
    {
        if (!_udp.valid()) {
            return;
        }
        _udp->disconnect();
    }

    // UDPDelegate overrides
    virtual void UDPevent(UDP* udp, Event, const char* data, uint16_t length) override;

private:
    Mad<UDP> _udp;
    Value _func;
    Value _parent;
    ExecutionUnit* _eu = nullptr;
};

}
