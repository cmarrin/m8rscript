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
#include "GC.h"
#include "TCP.h"

using namespace m8r;

static StaticObject::StaticFunctionProperty _props[] =
{
    { SA::constructor, IPAddrProto::constructor },
    { SA::toString, IPAddrProto::toString },
    { SA::lookupHostname, IPAddrProto::lookupHostname },

};

IPAddrProto::IPAddrProto()
{
    setProperties(_props, sizeof(_props) / sizeof(StaticFunctionProperty));
}

CallReturnValue IPAddrProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // Stack: string ip octets or 4 integers
    IPAddr ipAddr;
    if (nparams == 1) {
        ipAddr = IPAddr(eu->stack().top().toStringValue(eu));
    } else if (nparams == 4) {
        int32_t a = eu->stack().top(-3).toIntValue(eu);
        int32_t b = eu->stack().top(-2).toIntValue(eu);
        int32_t c = eu->stack().top(-1).toIntValue(eu);
        int32_t d = eu->stack().top().toIntValue(eu);
        if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) {
            return CallReturnValue(Error::Code::OutOfRange);
        }
        ipAddr[0] = a;
        ipAddr[1] = b;
        ipAddr[2] = c;
        ipAddr[3] = d;
    }
    
    Mad<Object> thisObject = thisValue.asObject();
    if (thisObject.valid()) {
        thisObject->setElement(eu, Value(0), Value(ipAddr[0]), Value::SetType::AlwaysAdd);
        thisObject->setElement(eu, Value(1), Value(ipAddr[1]), Value::SetType::AlwaysAdd);
        thisObject->setElement(eu, Value(2), Value(ipAddr[2]), Value::SetType::AlwaysAdd);
        thisObject->setElement(eu, Value(3), Value(ipAddr[3]), Value::SetType::AlwaysAdd);
    }
    
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue IPAddrProto::toString(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams != 0) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    IPAddr ipAddr;
    ipAddr[0] = thisValue.element(eu, Value(0)).toIntValue(eu);
    ipAddr[1] = thisValue.element(eu, Value(1)).toIntValue(eu);
    ipAddr[2] = thisValue.element(eu, Value(2)).toIntValue(eu);
    ipAddr[3] = thisValue.element(eu, Value(3)).toIntValue(eu);
    
    Mad<String> string = ExecutionUnit::createString(ipAddr.toString());
    eu->stack().push(Value(string));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue IPAddrProto::lookupHostname(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 2) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }

    Value hostnameValue = eu->stack().top(1 - nparams);
    String hostname = hostnameValue.toStringValue(eu);
    Value funcValue = eu->stack().top(2 - nparams);
    if (funcValue.asObject().valid()) {
        GC::addStaticObject(funcValue.asObject().raw());
    }
    
    eu->startEventListening();
            
    IPAddr::lookupHostName(hostname.c_str(), [thisValue, eu, funcValue](const char* name, m8r::IPAddr ipaddr) {
        Mad<Object> obj = ObjectFactory::create(SAtom(SA::IPAddr), eu, 0);
        obj->setElement(eu, Value(0), Value(ipaddr[0]), Value::SetType::AlwaysAdd);
        obj->setElement(eu, Value(1), Value(ipaddr[1]), Value::SetType::AlwaysAdd);
        obj->setElement(eu, Value(2), Value(ipaddr[2]), Value::SetType::AlwaysAdd);
        obj->setElement(eu, Value(3), Value(ipaddr[3]), Value::SetType::AlwaysAdd);

        Value args[2];
        args[0] = Value(ExecutionUnit::createString(name));
        args[1] = Value(obj);
        
        eu->fireEvent(funcValue, thisValue, args, 2);
        if (funcValue.asObject().valid()) {
            GC::removeStaticObject(funcValue.asObject().raw());
        }
        eu->stopEventListening();
    });
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
