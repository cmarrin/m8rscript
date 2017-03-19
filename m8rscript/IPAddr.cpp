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

IPAddrProto::IPAddrProto(Program* program)
    : ObjectFactory(program, ROMSTR("IPAddrProto"))
    , _constructor(constructor)
    , _lookupHostname(lookupHostname)
{
    addProperty(program, ATOM(constructor), &_constructor);
    addProperty(program, ATOM(lookupHostname), &_lookupHostname);
}

static bool toIPAddr(const String& ipString, IPAddr& ip)
{
    std::vector<String> array = ipString.split(".");
    if (array.size() != 4) {
        return false;
    }
    
    //for (auto s : array) {
        return false;
    //}
    return true;
}

CallReturnValue IPAddrProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // ip address string or 4 numbers
    IPAddr ipAddr;
    if (nparams == 1) {
        String ipString = eu->stack().top().toStringValue(eu);
        if (!toIPAddr(ipString, ipAddr)) {
            return CallReturnValue(CallReturnValue::Type::Error);
        }
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
    
    Object* obj = Global::obj(thisValue);
    if (!obj) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    obj->setProperty(eu, ATOM(a), Value(static_cast<int32_t>(ipAddr[0])), Object::SetPropertyType::AlwaysAdd);
    obj->setProperty(eu, ATOM(b), Value(static_cast<int32_t>(ipAddr[1])), Object::SetPropertyType::AlwaysAdd);
    obj->setProperty(eu, ATOM(c), Value(static_cast<int32_t>(ipAddr[2])), Object::SetPropertyType::AlwaysAdd);
    obj->setProperty(eu, ATOM(d), Value(static_cast<int32_t>(ipAddr[3])), Object::SetPropertyType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue IPAddrProto::lookupHostname(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    Object* obj = Global::obj(thisValue);
    if (!obj) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    MyTCPDelegate* delegate = reinterpret_cast<MyTCPDelegate*>(obj->property(eu, ATOM(__nativeObject)).asNativeObject());
    
    int16_t connectionId = eu->stack().top(1 - nparams).toIntValue(eu);
    delegate->disconnect(connectionId);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
