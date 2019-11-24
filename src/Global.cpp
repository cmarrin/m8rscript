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

Global::Global()
{
    static StaticObject::StaticProperty RODATA2_ATTR _props[] =
    {
        { SA::Base64, Value(&_base64) },
        { SA::GPIO, Value(&_gpio) },
        { SA::JSON, Value(&_json) },
        { SA::TCP, Value(&_tcp) },
        { SA::UDP, Value(&_udp) },
        { SA::IPAddr, Value(&_ipAddr) },
        { SA::Iterator, Value(&_iterator) },
        { SA::Task, Value(&_task) },
        { SA::FS, Value(&_fs) },
        { SA::File, Value(&_file) },
        { SA::Directory, Value(&_directory) },
        
        { SA::currentTime, Value(currentTime) },
        { SA::delay, Value(delay) },
        { SA::print, Value(print) },
        { SA::printf, Value(printf) },
        { SA::println, Value(println) },
        { SA::toFloat, Value(toFloat) },
        { SA::toInt, Value(toInt) },
        { SA::toUInt, Value(toUInt) },
        { SA::arguments, Value(arguments) },
        { SA::import, Value(import) },
        { SA::importString, Value(importString) },
        { SA::waitForEvent, Value(waitForEvent) },
        { SA::meminfo, Value(meminfo) },
    };
    setProperties(_props, sizeof(_props) / sizeof(StaticProperty));
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
        eu->print(s.c_str());
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
    eu->print(s.c_str());
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Global::println(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        eu->print(s.c_str());
    }
    eu->print("\n");

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
    Mad<Object> array = ObjectFactory::create(Atom(SA::Array), eu, 0);
    if (!array.valid()) {
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
    MemoryInfo info = Mallocator::shared()->memoryInfo();
    Mad<Object> obj = Mad<MaterObject>::create();
    
    uint32_t freeSize = info.freeSizeInBlocks * info.blockSize;
    uint32_t allocatedSize = (info.heapSizeInBlocks - info.freeSizeInBlocks) * info.blockSize;
    obj->setProperty(eu->program()->atomizeString(ROMSTR("freeSize")),
                     Value(static_cast<int32_t>(freeSize)), Value::SetPropertyType::AlwaysAdd);
                     
    obj->setProperty(eu->program()->atomizeString(ROMSTR("allocatedSize")),
                     Value(static_cast<int32_t>(allocatedSize)), Value::SetPropertyType::AlwaysAdd);
                     
    obj->setProperty(eu->program()->atomizeString(ROMSTR("numAllocations")),
                     Value(static_cast<int32_t>(info.numAllocations)), Value::SetPropertyType::AlwaysAdd);
                     
    Mad<Object> allocationsByType = Mad<MaterObject>::create();
    allocationsByType->setArray(true);
    for (int i = 0; i < info.allocationsByType.size(); ++i) {
        Mad<Object> allocation = Mad<MaterObject>::create();
        uint32_t size = info.allocationsByType[i].sizeInBlocks * info.blockSize;
        ROMString type = Mallocator::stringFromMemoryType(static_cast<MemoryType>(i));
        allocation->setProperty(eu->program()->atomizeString(ROMSTR("count")),
                         Value(static_cast<int32_t>(info.allocationsByType[i].count)), Value::SetPropertyType::AlwaysAdd);
        allocation->setProperty(eu->program()->atomizeString(ROMSTR("size")),
                         Value(static_cast<int32_t>(size)), Value::SetPropertyType::AlwaysAdd);
        allocation->setProperty(eu->program()->atomizeString(ROMSTR("type")),
                         Value(Mad<String>::create(type)), Value::SetPropertyType::AlwaysAdd);

        allocationsByType->setElement(eu, Value(0), Value(allocation), true);
    }
    
    obj->setProperty(eu->program()->atomizeString(ROMSTR("allocationsByType")),
                     Value(allocationsByType), Value::SetPropertyType::AlwaysAdd);

    eu->stack().push(Value(obj));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
