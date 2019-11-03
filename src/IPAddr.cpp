/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "IPAddr.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "Program.h"

using namespace m8r;

static bool toIPAddr(const String& ipString, IPAddr& ip)
{
    std::vector<String> array = ipString.split(".");
    if (array.size() != 4) {
        return false;
    }
    
    for (uint32_t i = 0; i < 4; ++i) {
        uint32_t v;
        if (!String::toUInt(v, array[i].c_str(), false) || v > 255) {
            return false;
        }
        ip[i] = static_cast<uint8_t>(v);
    }
    return true;
}

IPAddrProto::IPAddrProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::IPAddr, parent, constructor)
{
    addProperty(program, SA::toString, toString);
    addProperty(program, SA::lookupHostname, lookupHostname);

    _obj->setArray(true);
    _obj->resize(4);
    (*_obj)[0] = Value(0);
    (*_obj)[1] = Value(0);
    (*_obj)[2] = Value(0);
    (*_obj)[3] = Value(0);
}

IPAddr::IPAddr(const String& ipString)
{
    toIPAddr(ipString, *this);
}

String IPAddr::toString() const
{
    return String::toString(_addr[0]) + "." +
           String::toString(_addr[1]) + "." +
           String::toString(_addr[2]) + "." +
           String::toString(_addr[3]);
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
            return CallReturnValue(CallReturnValue::Error::OutOfRange);
        }
        ipAddr[0] = a;
        ipAddr[1] = b;
        ipAddr[2] = c;
        ipAddr[3] = d;
    } else {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Object* thisObject = thisValue.asObject();
    if (thisObject) {
        thisObject->setArray(true);
        thisObject->setElement(eu, Value(0), Value(ipAddr[0]), true);
        thisObject->setElement(eu, Value(1), Value(ipAddr[1]), true);
        thisObject->setElement(eu, Value(2), Value(ipAddr[2]), true);
        thisObject->setElement(eu, Value(3), Value(ipAddr[3]), true);
    }
    
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue IPAddrProto::toString(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams != 0) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    IPAddr ipAddr;
    ipAddr[0] = thisValue.element(eu, Value(0)).toIntValue(eu);
    ipAddr[1] = thisValue.element(eu, Value(1)).toIntValue(eu);
    ipAddr[2] = thisValue.element(eu, Value(2)).toIntValue(eu);
    ipAddr[3] = thisValue.element(eu, Value(3)).toIntValue(eu);
    
    String* string = Object::createString(ipAddr.toString());
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
    if (funcValue.asObject()) {
        Object::addStaticObject(funcValue.asObject());
    }
    
    eu->startEventListening();
    Object* thisObject = thisValue.asObject();
            
    IPAddr::lookupHostName(hostname.c_str(), [thisObject, eu, funcValue](const char* name, m8r::IPAddr ipaddr) {
        Object* obj = ObjectFactory::create(Atom(SA::IPAddr), eu, 0);
        obj->setElement(eu, Value(0), Value(ipaddr[0]), true);
        obj->setElement(eu, Value(1), Value(ipaddr[1]), true);
        obj->setElement(eu, Value(2), Value(ipaddr[2]), true);
        obj->setElement(eu, Value(3), Value(ipaddr[3]), true);

        Value args[2];
        args[0] = Value(Object::createString(name));
        args[1] = Value(obj);
        
        eu->fireEvent(funcValue, Value(thisObject), args, 2);
        if (funcValue.asObject()) {
            Object::removeStaticObject(funcValue.asObject());
        }
        eu->stopEventListening();
    });
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

void IPAddrProto::setIPAddr(IPAddr ipaddr)
{
    _obj->resize(4);
    (*_obj)[0] = Value(ipaddr[0]);
    (*_obj)[1] = Value(ipaddr[1]);
    (*_obj)[2] = Value(ipaddr[2]);
    (*_obj)[3] = Value(ipaddr[3]);
}
