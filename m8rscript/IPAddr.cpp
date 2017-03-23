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

String IPAddrProto::toString(ExecutionUnit* eu, bool typeOnly) const
{
    return typeOnly ? String("IPAddr") : _ipAddr.toString();
}

CallReturnValue IPAddrProto::construct(ExecutionUnit* eu, uint32_t nparams)
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
            return CallReturnValue(CallReturnValue::Type::Error);
        }
        ipAddr[0] = a;
        ipAddr[1] = b;
        ipAddr[2] = c;
        ipAddr[3] = d;
    } else {
        return CallReturnValue(CallReturnValue::Type::Error);
    }

    IPAddrProto* obj = new IPAddrProto();
    Global::addObject(obj, true);
    obj->setProto(objectId());
    
    obj->_ipAddr = ipAddr;

    eu->stack().push(Value(obj->objectId()));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue IPAddrProto::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    if (prop == ATOM(lookupHostname)) {
        if (nparams < 2) {
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }

        Value hostnameValue = eu->stack().top(1 - nparams);
        String hostname = hostnameValue.toStringValue(eu);
        Value func = eu->stack().top(2 - nparams);
        
        IPAddr::lookupHostName(hostname.c_str(), [this, eu, func](const char* name, m8r::IPAddr ipaddr) {
            ObjectId newIPAddr;
            Object* parent = Global::obj(objectId());
            if (parent) {
                IPAddrProto* obj = new IPAddrProto();
                Global::addObject(obj, true);
                obj->setProto(parent->objectId());
                obj->_ipAddr = ipaddr;
                newIPAddr = obj->objectId();
            }

            Value args[2];
            args[0] = Value(Global::createString(String(name)));
            args[1] = Value(newIPAddr);
            
            // FIXME: We need a this pointer
             eu->fireEvent(func, Value(), args, 2);
        });
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    return CallReturnValue(CallReturnValue::Type::Error);
}
