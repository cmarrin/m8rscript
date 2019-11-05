/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Global.h"

#include "ExecutionUnit.h"
#include "MStream.h"
#include "SystemInterface.h"

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
    , _task(program, this)
    , _fs(program, this)
    , _file(program, this)
    , _directory(program, this)
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
    addProperty(program, SA::waitForEvent, waitForEvent);
    addProperty(program, SA::meminfo, meminfo);

    addProperty(program, SA::Array, &_array);
    addProperty(program, SA::Object, &_object);
    
    addProperty(program, SA::consoleListener, Value::NullValue());
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
        String s = eu->stack().top(i).toStringValue(eu);
        if (eu->consolePrintFunction()) {
            eu->consolePrintFunction()(s);
        } else {
            system()->printf(s.c_str());
        }
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Global::printf(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::BadFormatString);
    }
    String s = Value::format(eu, eu->stack().top(1 - nparams), nparams - 1);
    if (s.empty()) {
        return CallReturnValue(CallReturnValue::Error::BadFormatString);
    }
    if (eu->consolePrintFunction()) {
        eu->consolePrintFunction()(s);
    } else {
        system()->printf(s.c_str());
    }
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Global::println(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        if (eu->consolePrintFunction()) {
            eu->consolePrintFunction()(s);

        } else {
            system()->printf(s.c_str());
        }
    }
    if (eu->consolePrintFunction()) {
        eu->consolePrintFunction()("\n");
    } else {
        system()->printf("\n");
    }

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
    if (String::toFloat(f, s.c_str(), allowWhitespace)) {
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
    if (String::toInt(i, s.c_str(), allowWhitespace)) {
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
    if (String::toUInt(u, s.c_str(), allowWhitespace)) {
        eu->stack().push(Value(static_cast<int32_t>(u)));
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
    }
    
    return CallReturnValue(CallReturnValue::Error::CannotConvertStringToNumber);
}

CallReturnValue Global::arguments(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Object* array = ObjectFactory::create(Atom(SA::Array), eu, 0);
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

CallReturnValue Global::waitForEvent(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::WaitForEvent);
}

CallReturnValue Global::meminfo(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    MemoryInfo info;
    system()->memoryInfo(info);
    Object* obj = new MaterObject();
    obj->setProperty(eu, eu->program()->atomizeString("freeSize"),
                     Value(static_cast<int32_t>(info.freeSize)), Value::SetPropertyType::AlwaysAdd);
    obj->setProperty(eu, eu->program()->atomizeString("numAllocations"),
                     Value(static_cast<int32_t>(info.numAllocations)), Value::SetPropertyType::AlwaysAdd);
    
    eu->stack().push(Value(obj));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
