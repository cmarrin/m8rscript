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

#include "TCPSocket.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"

using namespace m8r;

//TCPSocketProto::TCPSocketProto(Program* program)
//    : ObjectFactory(program, ROMSTR("TCPSocketProto"))
//    , _constructor(constructor)
//    , _send(send)
//    , _disconnect(disconnect)
//{
//    addProperty(program, ATOM(constructor), &_constructor);
//    addProperty(program, ATOM(send), &_send);
//    addProperty(program, ATOM(disconnect), &_disconnect);
//    
//    addProperty(program, ATOM(Connected), Value(static_cast<int32_t>(TCPDelegate::Event::Connected)));
//    addProperty(program, ATOM(Reconnected), Value(static_cast<int32_t>(TCPDelegate::Event::Reconnected)));
//    addProperty(program, ATOM(Disconnected), Value(static_cast<int32_t>(TCPDelegate::Event::Disconnected)));
//    addProperty(program, ATOM(ReceivedData), Value(static_cast<int32_t>(TCPDelegate::Event::ReceivedData)));
//    addProperty(program, ATOM(SentData), Value(static_cast<int32_t>(TCPDelegate::Event::SentData)));
//}
//
//CallReturnValue TCPSocketProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
//{
//    // If 2 params: port number, event function, 3 params is ip, port, func
//    if (nparams < 2) {
//        return CallReturnValue(CallReturnValue::Type::Error);
//    }
//    
//    int32_t port = -1;
//    Value ip;
//    Value func;
//    
//    if (nparams == 2) {
//        port = eu->stack().top(-1).toIntValue(eu);
//        func = eu->stack().top();
//    } else {
//        ip = eu->stack().top(1 - nparams);
//        port = eu->stack().top(2 - nparams).toIntValue(eu);
//        func = eu->stack().top(3 - nparams);
//    }
//
//    // FIXME: Support IP address (client mode)
//    MyTCPDelegate* delegate = new MyTCPDelegate(eu, IPAddr(), port, func, thisValue.asObjectIdValue());
//    
//    Object* obj = Global::obj(thisValue);
//    if (!obj) {
//        return CallReturnValue(CallReturnValue::Type::Error);
//    }
//    obj->setProperty(eu, ATOM(__nativeObject), Value(delegate), Object::SetPropertyType::AlwaysAdd);
//
//    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//}
//
//MyTCPDelegate::MyTCPDelegate(IPAddr ip, uint16_t port, const Value& func, const Value& parent)
//    : _func(func)
//    , _parent(parent)
//{
//    // FIXME: Implement client
//    assert(!ip);
//
//    _tcp = TCP::create(this, port);
//}
//
//CallReturnValue TCPSocketProto::send(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
//{
//    if (nparams < 1) {
//        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//    }
//    
//    Object* obj = Global::obj(thisValue);
//    if (!obj) {
//        return CallReturnValue(CallReturnValue::Type::Error);
//    }
//    
//    MyTCPDelegate* delegate = reinterpret_cast<MyTCPDelegate*>(obj->property(eu, ATOM(__nativeObject)).asNativeObject());
//    if (!delegate) {
//        return CallReturnValue(CallReturnValue::Type::Error);
//    }
//    
//    int16_t connectionId = eu->stack().top(1 - nparams).toIntValue(eu);
//    for (int32_t i = 2 - nparams; i <= 0; ++i) {
//        String s = eu->stack().top(i).toStringValue(eu);
//        delegate->send(connectionId, s.c_str(), s.size());
//    }
//    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//}
//
//CallReturnValue TCPSocketProto::disconnect(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
//{
//    if (nparams < 1) {
//        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//    }
//    
//    Object* obj = Global::obj(thisValue);
//    if (!obj) {
//        return CallReturnValue(CallReturnValue::Type::Error);
//    }
//    
//    MyTCPDelegate* delegate = reinterpret_cast<MyTCPDelegate*>(obj->property(eu, ATOM(__nativeObject)).asNativeObject());
//    
//    int16_t connectionId = eu->stack().top(1 - nparams).toIntValue(eu);
//    delegate->disconnect(connectionId);
//    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
//}
//
//void MyTCPDelegate::TCPevent(TCP* tcp, Event event, int16_t connectionId, const char* data, uint16_t length)
//{
//    Value args[4];
//    args[0] = Value(static_cast<int32_t>(event));
//    args[1] = Value(static_cast<int32_t>(connectionId));
//    
//    if (data) {
//        StringId dataString = Global::createString(data, length);
//        
//        args[2] = Value(dataString);
//        args[3] = Value(static_cast<int32_t>(length));
//    }
//    EventManager::shared()->fireEvent(_func, _parent, args, data ? 4 : 2);
//}




