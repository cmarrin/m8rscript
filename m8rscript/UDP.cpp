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

#include "UDP.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"

using namespace m8r;

UDPProto::UDPProto(Program* program)
    : ObjectFactory(program, ROMSTR("UDPProto"))
    , _constructor(constructor)
    , _send(send)
    , _disconnect(disconnect)
{
    addProperty(program, ATOM(constructor), &_constructor);
    addProperty(program, ATOM(send), &_send);
    
    addProperty(program, ATOM(ReceivedData), Value(static_cast<int32_t>(UDPDelegate::Event::ReceivedData)));
    addProperty(program, ATOM(SentData), Value(static_cast<int32_t>(UDPDelegate::Event::SentData)));
}

CallReturnValue UDPProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // port number, event function
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    int32_t port = -1;
    Value ip;
    Value func;
    
    if (nparams == 1) {
        port = eu->stack().top().toIntValue(eu);
    } else {
        port = eu->stack().top(1 - nparams).toIntValue(eu);
        func = eu->stack().top(2 - nparams);
    }

    MyUDPDelegate* delegate = new MyUDPDelegate(eu, port, func, thisValue.asObjectIdValue());
    
    Object* obj = Global::obj(thisValue);
    if (!obj) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    obj->setProperty(eu, ATOM(__nativeObject), Value(delegate), Object::SetPropertyType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

MyUDPDelegate::MyUDPDelegate(ExecutionUnit* eu, uint16_t port, const Value& func, const Value& parent)
    : _func(func)
    , _parent(parent)
    , _eu(eu)
{
    _udp = UDP::create(this, port);
}

CallReturnValue UDPProto::send(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // ip, port, data
    if (nparams < 3) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    Object* obj = Global::obj(thisValue);
    if (!obj) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    MyUDPDelegate* delegate = reinterpret_cast<MyUDPDelegate*>(obj->property(eu, ATOM(__nativeObject)).asNativeObject());
    if (!delegate) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    String ipString = eu->stack().top(1 - nparams).toStringValue(eu);
    IPAddr ip;
//    if (!toIPAddr(ipString, ip)) {
//        // String must be a host name
//        // FIXME: handle hostname
//        return CallReturnValue(CallReturnValue::Type::Error);
//    }
    
    int32_t port = eu->stack().top(2 - nparams).toIntValue(eu);
    for (int32_t i = 3 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        delegate->send(ip, port, s.c_str(), s.size());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue UDPProto::disconnect(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* obj = Global::obj(thisValue);
    if (!obj) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    MyUDPDelegate* delegate = reinterpret_cast<MyUDPDelegate*>(obj->property(eu, ATOM(__nativeObject)).asNativeObject());
    
    delegate->disconnect();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

void MyUDPDelegate::UDPevent(UDP* udp, Event event, const char* data, uint16_t length)
{
    Value args[4];
    args[0] = Value(static_cast<int32_t>(event));
    
    if (data) {
        StringId dataString = Global::createString(data, length);
        
        args[2] = Value(dataString);
        args[3] = Value(static_cast<int32_t>(length));
    }
    _eu->fireEvent(_func, _parent, args, data ? 4 : 2);
}