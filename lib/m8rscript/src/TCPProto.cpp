/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "TCPProto.h"

#include "ExecutionUnit.h"
#include "SystemInterface.h"
#include "TCP.h"

using namespace m8r;

static StaticObject::StaticFunctionProperty _functionProps[] =
{
    { SA::constructor, TCPProto::constructor },
    { SA::send, TCPProto::send },
    { SA::disconnect, TCPProto::disconnect },
    { SA::disconnect, TCPProto::disconnect },
};

static StaticObject::StaticProperty _props[] =
{
    { SA::Connected, Value(static_cast<int32_t>(TCP::Event::Connected)) },
    { SA::Reconnected, Value(static_cast<int32_t>(TCP::Event::Reconnected)) },
    { SA::Disconnected, Value(static_cast<int32_t>(TCP::Event::Disconnected)) },
    { SA::Error, Value(static_cast<int32_t>(TCP::Event::Error)) },
    { SA::ReceivedData, Value(static_cast<int32_t>(TCP::Event::ReceivedData)) },
    { SA::SentData, Value(static_cast<int32_t>(TCP::Event::SentData)) },
    { SA::MaxConnections, Value(static_cast<int32_t>(TCP::MaxConnections)) },
};

TCPProto::TCPProto()
{
    setProperties(_functionProps, sizeof(_functionProps) / sizeof(StaticFunctionProperty));
    setProperties(_props, sizeof(_props) / sizeof(StaticProperty));
}

CallReturnValue TCPProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // If 2 params: port number, event function, 3 params is ip, port, func
    if (nparams < 2) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
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

    Mad<TCP> tcp = system()->createTCP(port, ipAddr, 
    [thisValue, eu, func](TCP*, TCP::Event event, int16_t connectionId, const char* data, int16_t length) {
        Value args[5];
        args[0] = thisValue;
        args[1] = Value(static_cast<int32_t>(event));
        args[2] = Value(static_cast<int32_t>(connectionId));
        
        if (data) {
            Mad<String> dataString = ExecutionUnit::createString(data, length);
            
            args[3] = Value(dataString);
            args[4] = Value(static_cast<int32_t>(length));
        }
        eu->fireEvent(func, thisValue, args, data ? 5 : 3);
    });
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::MissingThis);
    }

    Mad<NativeObject> nativeDelegate = static_cast<Mad<NativeObject>>(tcp);
    obj->setProperty(SAtom(SA::__nativeObject), Value(nativeDelegate), Value::SetType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TCPProto::send(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    //
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::MissingThis);
    }
    
    Mad<TCP> tcp = thisValue.isObject() ? thisValue.asObject()->getNative<TCP>() : Mad<TCP>();
    if (!tcp.valid()) {
        return CallReturnValue(Error::Code::InternalError);
    }

    int16_t connectionId = eu->stack().top(1 - nparams).toIntValue(eu);
    for (int32_t i = 2 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        tcp->send(connectionId, s.c_str(), s.size());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TCPProto::disconnect(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::MissingThis);
    }
    
    Mad<TCP> tcp = thisValue.isObject() ? thisValue.asObject()->getNative<TCP>() : Mad<TCP>();
    if (!tcp.valid()) {
        return CallReturnValue(Error::Code::InternalError);
    }

    int16_t connectionId = nparams ? eu->stack().top(1 - nparams).toIntValue(eu) : 0;
    tcp->disconnect(connectionId);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
