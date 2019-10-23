/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
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
    UDP(UDPDelegate* delegate, uint16_t port) : _delegate(delegate), _port(port) { }

    UDPDelegate* _delegate;
    uint16_t _port;
};

// Object

class UDPProto : public ObjectFactory {
public:
    UDPProto(Program*, ObjectFactory* parent);

private:
    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue send(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue disconnect(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
    NativeFunction _send;
    NativeFunction _disconnect;
};

class MyUDPDelegate : public NativeObject, public UDPDelegate {
public:
    MyUDPDelegate(ExecutionUnit*, uint16_t port, const Value& func, const Value& parent);
    virtual ~MyUDPDelegate() { }

    void send(IPAddr ip, uint16_t port, const char* data, uint16_t size)
    {
        if (!_udp) {
            return;
        }
        _udp->send(ip, port, data, size);
    }
    
    void disconnect()
    {
        if (!_udp) {
            return;
        }
        _udp->disconnect();
    }

    // UDPDelegate overrides
    virtual void UDPevent(UDP* udp, Event, const char* data, uint16_t length) override;

private:
    std::unique_ptr<UDP> _udp;
    Value _func;
    Value _parent;
    ExecutionUnit* _eu;
};

}
