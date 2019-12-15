/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "TCP.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Mallocator.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

static StaticObject::StaticProperty RODATA2_ATTR _props[] =
{
    { SA::send, Value(TCPProto::send) },
    { SA::disconnect, Value(TCPProto::disconnect) },
    { SA::disconnect, Value(TCPProto::disconnect) },
    { SA::Connected, Value(static_cast<int32_t>(TCPDelegate::Event::Connected)) },
    { SA::Reconnected, Value(static_cast<int32_t>(TCPDelegate::Event::Reconnected)) },
    { SA::Disconnected, Value(static_cast<int32_t>(TCPDelegate::Event::Disconnected)) },
    { SA::Error, Value(static_cast<int32_t>(TCPDelegate::Event::Error)) },
    { SA::ReceivedData, Value(static_cast<int32_t>(TCPDelegate::Event::ReceivedData)) },
    { SA::SentData, Value(static_cast<int32_t>(TCPDelegate::Event::SentData)) },
    { SA::MaxConnections, Value(static_cast<int32_t>(TCP::MaxConnections)) },};

TCPProto::TCPProto()
{
    setProperties(_props, sizeof(_props) / sizeof(StaticProperty));
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
    Mad<Object> ipAddrObject = ipValue.asObject();
    if (ipAddrObject.valid()) {
        ipAddr[0] = ipAddrObject->element(eu, Value(0)).toIntValue(eu);
        ipAddr[1] = ipAddrObject->element(eu, Value(1)).toIntValue(eu);
        ipAddr[2] = ipAddrObject->element(eu, Value(2)).toIntValue(eu);
        ipAddr[3] = ipAddrObject->element(eu, Value(3)).toIntValue(eu);
    }

    Mad<MyTCPDelegate> delegate = Mad<MyTCPDelegate>::create();
    delegate->init(eu, ipAddr, port, func, thisValue);
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }

    Mad<NativeObject> nativeDelegate = static_cast<Mad<NativeObject>>(delegate);
    obj->setProperty(Atom(SA::__nativeObject), Value(nativeDelegate), Value::SetPropertyType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

void MyTCPDelegate::init(ExecutionUnit* eu, IPAddr ip, uint16_t port, const Value& func, const Value& parent)
{
    _func = func;
    _parent = parent;
    _eu = eu;
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
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Mad<MyTCPDelegate> delegate;
    CallReturnValue ret = getNative(delegate, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
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
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Mad<MyTCPDelegate> delegate;
    CallReturnValue ret = getNative(delegate, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
    }
    
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
        Mad<String> dataString = ExecutionUnit::createString(data, length);
        
        args[3] = Value(dataString);
        args[4] = Value(static_cast<int32_t>(length));
    }
    _eu->fireEvent(_func, _parent, args, data ? 5 : 3);
}




