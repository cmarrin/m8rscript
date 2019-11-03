/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "UDP.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

UDPProto::UDPProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::UDPProto, parent, constructor)
{
    addProperty(program, SA::send, send);
    addProperty(program, SA::disconnect, disconnect);
    
    addProperty(Atom(SA::ReceivedData), Value(static_cast<int32_t>(UDPDelegate::Event::ReceivedData)));
    addProperty(Atom(SA::SentData), Value(static_cast<int32_t>(UDPDelegate::Event::SentData)));
}

CallReturnValue UDPProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // port number, event function
    int32_t port = 0;
    Value ip;
    Value func;
    
    if (nparams == 1) {
        port = eu->stack().top().toIntValue(eu);
    } else if (nparams >= 2) {
        port = eu->stack().top(1 - nparams).toIntValue(eu);
        func = eu->stack().top(2 - nparams);
    }

    MyUDPDelegate* delegate = new MyUDPDelegate(eu, port, func, thisValue);
    
    Object* obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    obj->setProperty(eu, Atom(SA::__nativeObject), Value(delegate), Value::SetPropertyType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

MyUDPDelegate::MyUDPDelegate(ExecutionUnit* eu, uint16_t port, const Value& func, const Value& parent)
    : _func(func)
    , _parent(parent)
    , _eu(eu)
{
    _udp = system()->createUDP(this, port);
}

CallReturnValue UDPProto::send(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // ip, port, data
    if (nparams < 3) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    Object* obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    MyUDPDelegate* delegate = reinterpret_cast<MyUDPDelegate*>(obj->property(eu, Atom(SA::__nativeObject)).asNativeObject());
    if (!delegate) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }
    
    String ipString = eu->stack().top(1 - nparams).toStringValue(eu);
    IPAddr ip(ipString);
    
    int32_t port = eu->stack().top(2 - nparams).toIntValue(eu);
    for (int32_t i = 3 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        delegate->send(ip, port, s.c_str(), s.size());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue UDPProto::disconnect(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    MyUDPDelegate* delegate = reinterpret_cast<MyUDPDelegate*>(obj->property(eu, Atom(SA::__nativeObject)).asNativeObject());
    
    delegate->disconnect();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

void MyUDPDelegate::UDPevent(UDP* udp, Event event, const char* data, uint16_t length)
{
    if (!_func) {
        return;
    }
    
    Value args[2];
    args[0] = Value(static_cast<int32_t>(event));
    
    if (data) {
        String* dataString = Object::createString(data, length);
        args[1] = Value(dataString);
    }
    _eu->fireEvent(_func, _parent, args, data ? 2 : 1);
}
