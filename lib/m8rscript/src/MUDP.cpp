/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MUDP.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

static StaticObject::StaticFunctionProperty RODATA2_ATTR _functionProps[] =
{
    { SA::constructor, UDPProto::constructor },
    { SA::send, UDPProto::send },
    { SA::disconnect, UDPProto::disconnect },
};

static StaticObject::StaticProperty _props[] =
{
    { SA::ReceivedData, Value(static_cast<int32_t>(TCP::Event::ReceivedData)) },
    { SA::SentData, Value(static_cast<int32_t>(TCP::Event::SentData)) },
};

UDPProto::UDPProto()
{
    setProperties(_functionProps, sizeof(_functionProps) / sizeof(StaticFunctionProperty));
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

    Mad<UDP> udp = system()->createUDP(port, [func, thisValue, eu](UDP*, UDP::Event event, const char* data, int16_t length)
    {
        if (!func) {
            return;
        }
        
        Value args[2];
        args[0] = Value(static_cast<int32_t>(event));
        
        if (data) {
            Mad<String> dataString = ExecutionUnit::createString(data, length);
            args[1] = Value(dataString);
        }
        eu->fireEvent(func, thisValue, args, data ? 2 : 1);
    });
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Mad<NativeObject> nativeDelegate = static_cast<Mad<NativeObject>>(udp);
    obj->setProperty(Atom(SA::__nativeObject), Value(nativeDelegate), Value::SetType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
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
    
    Mad<UDP> udp = thisValue.isObject() ? thisValue.asObject()->getNative<UDP>() : Mad<UDP>();
    if (!udp.valid()) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }

    String ipString = eu->stack().top(1 - nparams).toStringValue(eu);
    IPAddr ip(ipString);
    
    int32_t port = eu->stack().top(2 - nparams).toIntValue(eu);
    for (int32_t i = 3 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        udp->send(ip, port, s.c_str(), s.size());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue UDPProto::disconnect(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Mad<UDP> udp = thisValue.isObject() ? thisValue.asObject()->getNative<UDP>() : Mad<UDP>();
    if (!udp.valid()) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }

    udp->disconnect();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
