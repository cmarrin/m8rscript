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

#include "TCP.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

TCPProto::TCPProto(Program* program)
    : ObjectFactory(program, ATOM(program, TCPProto))
    , _constructor(constructor)
    , _send(send)
    , _disconnect(disconnect)
{
    addProperty(ATOM(program, constructor), &_constructor);
    addProperty(ATOM(program, send), &_send);
    addProperty(ATOM(program, disconnect), &_disconnect);
    
    addProperty(ATOM(program, Connected), Value(static_cast<int32_t>(TCPDelegate::Event::Connected)));
    addProperty(ATOM(program, Reconnected), Value(static_cast<int32_t>(TCPDelegate::Event::Reconnected)));
    addProperty(ATOM(program, Disconnected), Value(static_cast<int32_t>(TCPDelegate::Event::Disconnected)));
    addProperty(ATOM(program, Error), Value(static_cast<int32_t>(TCPDelegate::Event::Error)));
    addProperty(ATOM(program, ReceivedData), Value(static_cast<int32_t>(TCPDelegate::Event::ReceivedData)));
    addProperty(ATOM(program, SentData), Value(static_cast<int32_t>(TCPDelegate::Event::SentData)));
    addProperty(ATOM(program, MaxConnections), Value(static_cast<int32_t>(TCP::MaxConnections)));
}

CallReturnValue TCPProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // If 2 params: port number, event function, 3 params is ip, port, func
    if (nparams < 2) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    int32_t port = -1;
    Value ipValue;
    Value func;
    
    if (nparams == 2) {
        port = eu->stack().top(-1).toIntValue(eu);
        func = eu->stack().top();
    } else {
        ipValue = eu->stack().top(1 - nparams);
        port = eu->stack().top(2 - nparams).toIntValue(eu);
        func = eu->stack().top(3 - nparams);
    }
    
    IPAddr ipAddr;
    Object* ipAddrObject = ipValue.asObject();
    if (ipAddrObject) {
        ipAddr[0] = ipAddrObject->element(eu, Value(0)).toIntValue(eu);
        ipAddr[1] = ipAddrObject->element(eu, Value(1)).toIntValue(eu);
        ipAddr[2] = ipAddrObject->element(eu, Value(2)).toIntValue(eu);
        ipAddr[3] = ipAddrObject->element(eu, Value(3)).toIntValue(eu);
    }

    MyTCPDelegate* delegate = new MyTCPDelegate(eu, ipAddr, port, func, thisValue);
    
    Object* obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    obj->setProperty(eu, ATOM(eu, __nativeObject), Value(delegate), Value::SetPropertyType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

MyTCPDelegate::MyTCPDelegate(ExecutionUnit* eu, IPAddr ip, uint16_t port, const Value& func, const Value& parent)
    : _func(func)
    , _parent(parent)
    , _eu(eu)
{
    _tcp = system()->createTCP(this, port, ip);
    _eu->startEventListening();
}

MyTCPDelegate::~MyTCPDelegate()
{
    _eu->stopEventListening();
}

CallReturnValue TCPProto::send(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    //
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    Object* obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    MyTCPDelegate* delegate = reinterpret_cast<MyTCPDelegate*>(obj->property(eu, ATOM(eu, __nativeObject)).asNativeObject());
    if (!delegate) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }
    
    int16_t connectionId = eu->stack().top(1 - nparams).toIntValue(eu);
    for (int32_t i = 2 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        delegate->send(connectionId, s.c_str(), s.size());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TCPProto::disconnect(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    MyTCPDelegate* delegate = reinterpret_cast<MyTCPDelegate*>(obj->property(eu, ATOM(eu, __nativeObject)).asNativeObject());
    
    int16_t connectionId = nparams ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
    delegate->disconnect(connectionId);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

void MyTCPDelegate::TCPevent(TCP* tcp, Event event, int16_t connectionId, const char* data, int16_t length)
{
    Value args[5];
    args[0] = _parent;
    args[1] = Value(static_cast<int32_t>(event));
    args[2] = Value(static_cast<int32_t>(connectionId));
    
    if (data) {
        String* dataString = Object::createString(data, length);
        
        args[3] = Value(dataString);
        args[4] = Value(static_cast<int32_t>(length));
    }
    _eu->fireEvent(_func, _parent, args, data ? 5 : 3);
}




