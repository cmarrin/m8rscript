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

#include "ExecutionUnit.h"
#include "MStream.h"
#include "SystemInterface.h"
#include "slre.h"
#include <string>

using namespace m8r;

//static const char* IteratorString = ROMSTR(
//   "class Iterator {\n"
//   "    var _obj;\n"
//   "    var _index;\n"
//   "    constructor(obj) { _obj = obj; _index = 0; }\n"
//   "    function done() { return _index >= _obj.length; }\n"
//   "    function next() { if (!done()) ++_index; }\n"
//   "    function getValue() { return done() ? null : _obj[_index]; }\n"
//   "    function setValue(v) { if (!done()) _obj[_index] = v; }\n"
//   "};\n"
//);

Global::Global(Program* program)
    : ObjectFactory(program, SA::Global)
    , _array(true)
    , _base64(program, this)
    , _gpio(program, this)
    , _json(program, this)
    , _tcp(program, this)
    , _udp(program, this)
    , _ipAddr(program, this)
    , _iterator(program, this)
{
    // The proto for IPAddr contains the local IP address
    _ipAddr.setIPAddr(IPAddr::myIPAddr());
    
    addProperty(program, SA::currentTime, currentTime);
    addProperty(program, SA::delay, delay);
    addProperty(program, SA::print, print);
    addProperty(program, SA::printf, printf);
    addProperty(program, SA::println, println);
    addProperty(program, SA::toFloat, toFloat);
    addProperty(program, SA::toInt, toInt);
    addProperty(program, SA::toUInt, toUInt);
    addProperty(program, SA::arguments, arguments);
    addProperty(program, SA::import, import);
    addProperty(program, SA::importString, importString);
    
    addProperty(program, SA::Array, &_array);
    addProperty(program, SA::Object, &_object);
    addProperty(program, SA::FS, system()->fileSystem());
}

Global::~Global()
{
}

CallReturnValue Global::currentTime(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint64_t t = static_cast<uint64_t>(Time::now());
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
        system()->printf(eu->stack().top(i).toStringValue(eu).c_str());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Global::printf(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    static const char* formatRegexROM = ROMSTR("(%)([\\d]*)(.?)([\\d]*)([c|s|d|i|x|X|u|f|e|E|g|G|p])");
    
    size_t formatRegexSize = ROMstrlen(formatRegexROM);
    char* formatRegex = new char[formatRegexSize];
    ROMmemcpy(formatRegex, formatRegexROM, formatRegexSize);
    
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
            system()->printf(s);
            delete [ ] formatRegex;
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        if (next < 0) {
            delete [ ] formatRegex;
            return CallReturnValue(CallReturnValue::Error::BadFormatString);
        }
        
        // Output anything from s to the '%'
        assert(caps[0].len == 1);
        if (s != caps[0].ptr) {
            String str(s, static_cast<int32_t>(caps[0].ptr - s));
            system()->printf(str.c_str());
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
                system()->printf(ROMSTR("%c"), value.toIntValue(eu));
                break;
            case 's':
                system()->printf(ROMSTR("%s"), value.toStringValue(eu).c_str());
                break;
            case 'd':
            case 'i':
                format += "d";
                system()->printf(format.c_str(), value.toIntValue(eu));
                break;
            case 'x':
            case 'X':
                format += (formatChar == 'x') ? "x" : "X";
                system()->printf(format.c_str(), static_cast<uint32_t>(value.toIntValue(eu)));
                break;
            case 'u':
                format += "u";
                system()->printf(format.c_str(), static_cast<uint32_t>(value.toIntValue(eu)));
                break;
            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                system()->printf(ROMSTR("%s"), value.toStringValue(eu).c_str());
                break;
            case 'p':
                system()->printf(ROMSTR("%p"), *(reinterpret_cast<void**>(&value)));
                break;
            default:
                delete [ ] formatRegex;
                return CallReturnValue(CallReturnValue::Error::UnknownFormatSpecifier);
        }
        
        s += next;
    }
}

CallReturnValue Global::println(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        system()->printf(eu->stack().top(i).toStringValue(eu).c_str());
    }
    system()->printf(ROMSTR("\n"));

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
    Object* array = ObjectFactory::create(ATOM(eu, SA::Array), eu, 0);
    if (!array) {
        return CallReturnValue(CallReturnValue::Error::CannotCreateArgumentsArray);
    }
    
    for (int32_t i = 0; i < eu->argumentCount(); ++i) {
        array->setElement(eu, Value(i), eu->argument(i), true);
    }
    eu->stack().push(Value(array));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Global::import(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // Library is loaded from a Stream or a string which is a filename
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    return eu->import(FileStream(system()->fileSystem(), s.c_str()), thisValue);
}

CallReturnValue Global::importString(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // Library is loaded from a Stream or a string which is a filename
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    return eu->import(StringStream(s), thisValue);
}
