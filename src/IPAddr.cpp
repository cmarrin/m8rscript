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
        if (!Value::toUInt(v, array[i].c_str(), false) || v > 255) {
            return false;
        }
        ip[i] = static_cast<uint8_t>(v);
    }
    return true;
}

IPAddrProto::IPAddrProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::IPAddr, parent, constructor)
    , _toString(toString, program, SA::toString, this)
    , _lookupHostname(lookupHostname, program, SA::lookupHostname, this)
{
}

IPAddr::IPAddr(const String& ipString)
{
    toIPAddr(ipString, *this);
}

String IPAddr::toString() const
{
    return Value::toString(_addr[0]) + "." +
           Value::toString(_addr[1]) + "." +
           Value::toString(_addr[2]) + "." +
           Value::toString(_addr[3]);
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
        Object* obj = ObjectFactory::create(ATOM(eu, SA::IPAddr), eu, 0);
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
