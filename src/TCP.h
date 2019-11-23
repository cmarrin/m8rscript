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

class TCPDelegate : public NativeObject {
public:
    virtual ~TCPDelegate() { }
    
    enum class Event { Connected, Reconnected, Disconnected, ReceivedData, SentData, Error };
    
    virtual void TCPevent(TCP*, Event, int16_t connectionId, const char* data = nullptr, int16_t length = -1) = 0;
};

class TCP {
    friend class MyEventTask;
    
public:
    static constexpr int MaxConnections = 4;
    static constexpr uint32_t DefaultTimeout = 7200;
     
    virtual ~TCP() { }
    
    virtual void send(int16_t connectionId, char c) = 0;
    virtual void send(int16_t connectionId, const char* data, uint16_t length = 0) = 0;
    virtual void disconnect(int16_t connectionId) = 0;

protected:
    void init(TCPDelegate* delegate, uint16_t port, IPAddr ip = IPAddr())
    {
        _delegate = delegate;
        _ip = ip;
        _port = port; 
    }

    TCPDelegate* _delegate;
    IPAddr _ip;
    uint16_t _port;
};

// Object

class TCPProto : public ObjectFactory {
public:
    TCPProto(ObjectFactory* parent);

private:
    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue send(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue disconnect(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

class MyTCPDelegate : public TCPDelegate {
public:
    MyTCPDelegate() { }
    virtual ~MyTCPDelegate();

    void init(ExecutionUnit*, IPAddr ip, uint16_t port, const Value& func, const Value& parent);

    virtual void gcMark() override
    {
        _func.gcMark();
        _parent.gcMark();
    }

    void send(int16_t connectionId, const char* data, uint16_t size)
    {
        if (!_tcp.valid()) {
            return;
        }
        _tcp->send(connectionId, data, size);
    }

    void disconnect(int16_t connectionId)
    {
        if (!_tcp.valid()) {
            return;
        }
        _tcp->disconnect(connectionId);
    }

    // TCPDelegate overrides
    virtual void TCPevent(TCP* tcp, Event, int16_t connectionId, const char* data, int16_t length) override;

private:
    Mad<TCP> _tcp;
    Value _func;
    Value _parent;
    ExecutionUnit* _eu;
};

}
