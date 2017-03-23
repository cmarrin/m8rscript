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

#include "Program.h"
#include "SystemInterface.h"
#include "ExecutionUnit.h"
#include "slre.h"
#include <string>

using namespace m8r;

Global::IdStore<StringId, String> Global::_stringStore;
Global::IdStore<ObjectId, Object> Global::_objectStore;

Global::Global(Program* program)
    : ObjectFactory(program, ROMSTR("Global"))
    , _array(program)
    , _base64(program)
    , _gpio(program)
    , _iterator(program)
    , _tcp(program)
    , _udp(program)
    , _ipAddr(program)
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
    // Add a dummy String to the start of _strings so we have something to return when a bad id is requested
    if (_stringStore.empty()) {
        StringId dummy = Global::createString("*** ERROR:INVALID STRING ***");
        assert(dummy.raw() == 0);
    }
    
    addProperty(program, ATOM(currentTime), &_currentTime);
    addProperty(program, ATOM(delay), &_delay);
    addProperty(program, ATOM(print), &_print);
    addProperty(program, ATOM(printf), &_printf);
    addProperty(program, ATOM(println), &_println);
    addProperty(program, ATOM(toFloat), &_toFloat);
    addProperty(program, ATOM(toInt), &_toInt);
    addProperty(program, ATOM(toUInt), &_toUInt);
    addProperty(program, ATOM(arguments), &_arguments);

    addProperty(program, ATOM(Array), &_array);
    addProperty(program, ATOM(Iterator), &_iterator);
    addProperty(program, ATOM(IPAddr), &_ipAddr);
    
    Global::addObject(_base64.nativeObject(), false);
    addProperty(program, ATOM(Base64), Value(_base64.objectId()));
    Global::addObject(_gpio.nativeObject(), false);
    addProperty(program, ATOM(GPIO), Value(_gpio.objectId()));
    Global::addObject(_tcp.nativeObject(), false);
    addProperty(program, ATOM(TCP), Value(_tcp.objectId()));
    Global::addObject(_udp.nativeObject(), false);
    addProperty(program, ATOM(UDP), Value(_udp.objectId()));
}

Global::~Global()
{
    Global::removeObject(_base64.nativeObject()->objectId());
    Global::removeObject(_gpio.nativeObject()->objectId());
    Global::removeObject(_tcp.nativeObject()->objectId());
    Global::removeObject(_udp.nativeObject()->objectId());
}

ObjectId Global::addObject(Object* obj, bool collectable)
{
    assert(!obj->objectId());
    obj->setCollectable(collectable);
    ObjectId id = _objectStore.add(obj);
    obj->setObjectId(id);
    return id;
}

void Global::removeObject(ObjectId objectId)
{
    assert(_objectStore.ptr(objectId)->objectId() == objectId);
    _objectStore.remove(objectId, false);
}

StringId Global::createString(const char* s, int32_t length)
{
    return _stringStore.add(new String(s, length));
}

StringId Global::createString(const String& s)
{
    return _stringStore.add(new String(s));
}

void Global::gc(ExecutionUnit* eu)
{
    _stringStore.gcClear();
    _objectStore.gcClear();
    
    // Mark string 0 (the dummy string)
    _stringStore.gcMark(StringId(0));
    
    eu->gcMark();
    
    _stringStore.gcSweep();
    _objectStore.gcSweep();
}

void Global::gcMark(ExecutionUnit* eu, const ObjectId& objectId)
{
    if (objectId && !_objectStore.isGCMarked(objectId)) {
        _objectStore.gcMark(objectId);
        obj(objectId)->gcMark(eu);
    }
}

void Global::gcMark(ExecutionUnit* eu, const Value& value)
{
    StringId stringId = value.asStringIdValue();
    if (stringId) {
        _stringStore.gcMark(stringId);
        return;
    }
    
    ObjectId objectId = value.asObjectIdValue();
    gcMark(eu, objectId);
}

CallReturnValue Global::currentTime(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint64_t t = eu->system()->currentMicroseconds();
    eu->stack().push(Float(static_cast<Float::value_type>(t), -6));
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
    struct slre_cap caps[5];
    memset(caps, 0, sizeof(caps));
    while (true) {
        int next = slre_match(formatRegex, s, size - static_cast<int>(s - start), caps, 5, 0);
        if (nextParam > 0 || next == SLRE_NO_MATCH) {
            // Print the remainder of the string
            eu->system()->printf(s);
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        if (next < 0) {
            return CallReturnValue(CallReturnValue::Type::Error);
        }
        
        // Output anything from s to the '%'
        assert(caps[0].len == 1);
        if (s != caps[0].ptr) {
            String str(s, static_cast<int32_t>(caps[0].ptr - s));
            eu->system()->printf(str.c_str());
        }
        
        // FIXME: handle the leading number(s) in the format
        assert(caps[4].len == 1);
        Value value = eu->stack().top(nextParam++);
        char formatChar = *(caps[4].ptr);
        switch (formatChar) {
            case 'c':
                eu->system()->printf("%c", value.toIntValue(eu));
                break;
            case 's':
                eu->system()->printf("%s", value.toStringValue(eu).c_str());
                break;
            case 'd':
            case 'i':
                eu->system()->printf("%d", value.toIntValue(eu));
                break;
            case 'x':
            case 'X':
                eu->system()->printf((formatChar == 'x') ? "%x" : "%X", static_cast<uint32_t>(value.toIntValue(eu)));
                break;
            case 'u':
                eu->system()->printf("%u", static_cast<uint32_t>(value.toIntValue(eu)));
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
            default: return CallReturnValue(CallReturnValue::Type::Error);
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
    
    return CallReturnValue(CallReturnValue::Type::Error);
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
    
    return CallReturnValue(CallReturnValue::Type::Error);
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
    
    return CallReturnValue(CallReturnValue::Type::Error);
}

CallReturnValue Global::arguments(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    ObjectId arrayId = ObjectFactory::create(ATOM(Array), eu, 0);
    Object* array = obj(arrayId);
    if (!array) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    for (uint32_t i = 0; i < eu->argumentCount(); ++i) {
        array->setIteratedValue(eu, i, eu->argument(i), Array::SetPropertyType::AlwaysAdd);
    }
    eu->stack().push(Value(arrayId));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
