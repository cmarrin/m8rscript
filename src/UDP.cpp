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

static StaticObject::StaticProperty RODATA2_ATTR _props[] =
{
    { SA::send, Value(UDPProto::send) },
    { SA::disconnect, Value(UDPProto::disconnect) },
    { SA::ReceivedData, Value(static_cast<int32_t>(TCPDelegate::Event::ReceivedData)) },
    { SA::SentData, Value(static_cast<int32_t>(TCPDelegate::Event::SentData)) },
};

UDPProto::UDPProto()
{
    setProperties(_props, sizeof(_props) / sizeof(StaticProperty));
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

    Mad<MyUDPDelegate> delegate = Mad<MyUDPDelegate>::create();
    delegate->init(eu, port, func, thisValue);
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Mad<NativeObject> nativeDelegate = static_cast<Mad<NativeObject>>(delegate);
    obj->setProperty(Atom(SA::__nativeObject), Value(nativeDelegate), Value::SetPropertyType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

void MyUDPDelegate::init(ExecutionUnit* eu, uint16_t port, const Value& func, const Value& parent)
{
    _func = func;
    _parent = parent;
    _eu = eu;
    _udp = system()->createUDP(this, port);
}

CallReturnValue UDPProto::send(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // ip, port, data
    if (nparams < 3) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Mad<MyUDPDelegate> delegate;
    CallReturnValue ret = getNative(delegate, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
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
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Mad<MyUDPDelegate> delegate;
    CallReturnValue ret = getNative(delegate, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
    }
    
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
        Mad<String> dataString = ExecutionUnit::createString(data, length);
        args[1] = Value(dataString);
    }
    _eu->fireEvent(_func, _parent, args, data ? 2 : 1);
}
