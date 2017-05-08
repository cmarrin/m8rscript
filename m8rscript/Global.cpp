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

#include "Global.h"

#include "SystemInterface.h"
#include "ExecutionUnit.h"
#include "slre.h"
#include <string>

using namespace m8r;

Global::Global(Program* program)
    : ObjectFactory(program, ATOM(program, Global))
    , _array(true)
    , _base64(program)
    , _gpio(program)
    , _json(program)
    , _tcp(program)
    , _udp(program)
    , _iterator(program)
    , _currentTime(currentTime)
    , _delay(delay)
    , _print(print)
    , _printf(printf)
    , _println(println)
    , _toFloat(toFloat)
    , _toInt(toInt)
    , _toUInt(toUInt)
    , _arguments(arguments)
{
    // The proto for IPAddr contains the local IP address
    _ipAddr.setIPAddr(IPAddr::myIPAddr());
    
    addProperty(ATOM(program, currentTime), &_currentTime);
    addProperty(ATOM(program, delay), &_delay);
    addProperty(ATOM(program, print), &_print);
    addProperty(ATOM(program, printf), &_printf);
    addProperty(ATOM(program, println), &_println);
    addProperty(ATOM(program, toFloat), &_toFloat);
    addProperty(ATOM(program, toInt), &_toInt);
    addProperty(ATOM(program, toUInt), &_toUInt);
    addProperty(ATOM(program, arguments), &_arguments);

    addProperty(ATOM(program, Array), &_array);
    addProperty(ATOM(program, Object), &_object);
    addProperty(ATOM(program, IPAddr), &_ipAddr);
    
    addProperty(ATOM(program, Base64), Value(_base64.nativeObject()));
    addProperty(ATOM(program, GPIO), Value(_gpio.nativeObject()));
    addProperty(ATOM(program, JSON), Value(_json.nativeObject()));
    addProperty(ATOM(program, TCP), Value(_tcp.nativeObject()));
    addProperty(ATOM(program, UDP), Value(_udp.nativeObject()));
    addProperty(ATOM(program, Iterator), Value(_iterator.nativeObject()));
}

Global::~Global()
{
}

CallReturnValue Global::currentTime(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint64_t t = SystemInterface::currentMicroseconds();
    eu->stack().push(Value(Float(static_cast<Float::value_type>(t), -6)));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Global::delay(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint32_t ms = eu->stack().top().toIntValue(eu);
    return CallReturnValue(CallReturnValue::Type::MsDelay, ms);
}

CallReturnValue Global::print(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        eu->system()->printf(eu->stack().top(i).toStringValue(eu).c_str());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Global::printf(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    static const char* formatRegex = "(%)([\\d]*)(.?)([\\d]*)([c|s|d|i|x|X|u|f|e|E|g|G|p])";
    
    int32_t nextParam = 1 - nparams;

    String format = eu->stack().top(nextParam++).toStringValue(eu);
    int size = static_cast<int>(format.size());
    const char* start = format.c_str();
    const char* s = start;
    while (true) {
        struct slre_cap caps[5];
        memset(caps, 0, sizeof(caps));
        int next = slre_match(formatRegex, s, size - static_cast<int>(s - start), caps, 5, 0);
        if (nextParam > 0 || next == SLRE_NO_MATCH) {
            // Print the remainder of the string
            eu->system()->printf(s);
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        if (next < 0) {
            return CallReturnValue(CallReturnValue::Error::BadFormatString);
        }
        
        // Output anything from s to the '%'
        assert(caps[0].len == 1);
        if (s != caps[0].ptr) {
            String str(s, static_cast<int32_t>(caps[0].ptr - s));
            eu->system()->printf(str.c_str());
        }
        
        // FIXME: handle the leading number(s) in the format
        assert(caps[4].len == 1);
        
        uint32_t width = 0;
        bool zeroFill = false;
        if (caps[1].len) {
            Value::toUInt(width, caps[1].ptr);
            if (caps[1].ptr[0] == '0') {
                zeroFill = true;
            }
        }
        
        Value value = eu->stack().top(nextParam++);
        char formatChar = *(caps[4].ptr);
        String format = String("%") + (zeroFill ? "0" : "") + (width ? Value::toString(width).c_str() : "");
        
        switch (formatChar) {
            case 'c':
                eu->system()->printf("%c", value.toIntValue(eu));
                break;
            case 's':
                eu->system()->printf("%s", value.toStringValue(eu).c_str());
                break;
            case 'd':
            case 'i':
                format += "d";
                eu->system()->printf(format.c_str(), value.toIntValue(eu));
                break;
            case 'x':
            case 'X':
                format += (formatChar == 'x') ? "x" : "X";
                eu->system()->printf(format.c_str(), static_cast<uint32_t>(value.toIntValue(eu)));
                break;
            case 'u':
                format += "u";
                eu->system()->printf(format.c_str(), static_cast<uint32_t>(value.toIntValue(eu)));
                break;
            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                eu->system()->printf("%s", value.toStringValue(eu).c_str());
                break;
            case 'p':
                eu->system()->printf("%p", *(reinterpret_cast<void**>(&value)));
                break;
            default: return CallReturnValue(CallReturnValue::Error::UnknownFormatSpecifier);
        }
        
        s += next;
    }
}

CallReturnValue Global::println(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        eu->system()->printf(eu->stack().top(i).toStringValue(eu).c_str());
    }
    eu->system()->printf("\n");

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Global::toFloat(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // string, allowWhitespace
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    bool allowWhitespace = true;
    if (nparams > 1) {
        allowWhitespace = eu->stack().top(2 - nparams).toIntValue(eu) != 0;
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    Float f;
    if (Value::toFloat(f, s.c_str(), allowWhitespace)) {
        eu->stack().push(Value(f));
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
    }
    
    return CallReturnValue(CallReturnValue::Error::CannotConvertStringToNumber);
}

CallReturnValue Global::toInt(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // string, allowWhitespace
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    bool allowWhitespace = true;
    if (nparams > 1) {
        allowWhitespace = eu->stack().top(2 - nparams).toIntValue(eu) != 0;
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    int32_t i;
    if (Value::toInt(i, s.c_str(), allowWhitespace)) {
        eu->stack().push(Value(i));
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
    }
    
    return CallReturnValue(CallReturnValue::Error::CannotConvertStringToNumber);
}

CallReturnValue Global::toUInt(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // string, allowWhitespace
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    bool allowWhitespace = true;
    if (nparams > 1) {
        allowWhitespace = eu->stack().top(2 - nparams).toIntValue(eu) != 0;
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    uint32_t u;
    if (Value::toUInt(u, s.c_str(), allowWhitespace)) {
        eu->stack().push(Value(static_cast<int32_t>(u)));
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
    }
    
    return CallReturnValue(CallReturnValue::Error::CannotConvertStringToNumber);
}

CallReturnValue Global::arguments(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* array = ObjectFactory::create(ATOM(eu, Array), eu, 0);
    if (!array) {
        return CallReturnValue(CallReturnValue::Error::CannotCreateArgumentsArray);
    }
    
    for (int32_t i = 0; i < eu->argumentCount(); ++i) {
        array->setElement(eu, Value(i), eu->argument(i), true);
    }
    eu->stack().push(Value(array));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
