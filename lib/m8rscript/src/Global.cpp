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

Global Global::_global;

Base64 Global::_base64;
GPIO Global::_gpio;
JSON Global::_json;
TCPProto Global::_tcp;
UDPProto Global::_udp;
IPAddrProto Global::_ipAddr;
Iterator Global::_iterator;
TaskProto Global::_task;
TimerProto Global::_timer;
FSProto Global::_fs;
FileProto Global::_file;
DirectoryProto Global::_directory;

static StaticObject::StaticFunctionProperty RODATA2_ATTR _functionProps[] =
{
    { SA::currentTime, Global::currentTime },
    { SA::delay, Global::delay },
    { SA::print, Global::print },
    { SA::println, Global::println },
    { SA::toFloat, Global::toFloat },
    { SA::toInt, Global::toInt },
    { SA::toUInt, Global::toUInt },
    { SA::arguments, Global::arguments },
    { SA::import, Global::import },
    { SA::importString, Global::importString },
    { SA::waitForEvent, Global::waitForEvent },
    { SA::meminfo, Global::meminfo },
};

static StaticObject::StaticObjectProperty RODATA2_ATTR _objectProps[] =
{
    { SA::Base64, &Global::_base64 },
    { SA::GPIO, &Global::_gpio },
    { SA::JSON, &Global::_json },
    { SA::TCP, &Global::_tcp },
    { SA::UDP, &Global::_udp },
    { SA::IPAddr, &Global::_ipAddr },
    { SA::Iterator, &Global::_iterator },
    { SA::Task, &Global::_task },
    { SA::Timer, &Global::_timer },
    { SA::FS, &Global::_fs },
    { SA::File, &Global::_file },
    { SA::Directory, &Global::_directory },
};

Global::Global()
{
    setProperties(_functionProps, sizeof(_functionProps) / sizeof(StaticFunctionProperty));
    setProperties(_objectProps, sizeof(_objectProps) / sizeof(StaticObjectProperty));
}

CallReturnValue Global::currentTime(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    uint64_t t = Time::now().us();
    eu->stack().push(Value(Float(static_cast<Float::value_type>(t), -6)));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue Global::delay(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Duration duration(eu->stack().top().toFloatValue(eu));
    if (!duration) {
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    return CallReturnValue(CallReturnValue::Type::Delay, duration.ms());
}

CallReturnValue Global::print(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    for (int32_t i = 1 - nparams; i <= 0; ++i) {
        String s = eu->stack().top(i).toStringValue(eu);
        eu->print(s.c_str());
    }
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
    
    for (int32_t i = 0; i < static_cast<int32_t>(eu->argumentCount()); ++i) {
        array->setElement(eu, Value(i), eu->argument(i), Value::SetType::AlwaysAdd);
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
    Mad<File> file = system()->fileSystem()->open(s.c_str(), FS::FileOpenMode::Read);
    CallReturnValue ret = eu->import(FileStream(file), thisValue);
    file.destroy(MemoryType::Native);
    return ret;
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
    Mad<Object> obj = Object::create<MaterObject>();
    
    uint32_t freeSize = Mallocator::shared()->freeSize();
    uint32_t allocatedSize = info.totalAllocatedBytes;
    obj->setProperty(eu->program()->atomizeString(ROMSTR("freeSize")),
                     Value(static_cast<int32_t>(freeSize)), Value::SetType::AlwaysAdd);
                     
    obj->setProperty(eu->program()->atomizeString(ROMSTR("allocatedSize")),
                     Value(static_cast<int32_t>(allocatedSize)), Value::SetType::AlwaysAdd);
                     
    obj->setProperty(eu->program()->atomizeString(ROMSTR("numAllocations")),
                     Value(static_cast<int32_t>(info.numAllocations)), Value::SetType::AlwaysAdd);
                     
    Mad<Object> allocationsByType = Object::create<MaterArray>();
    for (uint32_t i = 0; i < info.allocationsByType.size(); ++i) {
        Mad<Object> allocation = Object::create<MaterObject>();
        uint32_t size = info.allocationsByType[i].size;
        ROMString type = Mallocator::stringFromMemoryType(static_cast<MemoryType>(i));
        allocation->setProperty(eu->program()->atomizeString(ROMSTR("count")),
                         Value(static_cast<int32_t>(info.allocationsByType[i].count)), Value::SetType::AlwaysAdd);
        allocation->setProperty(eu->program()->atomizeString(ROMSTR("size")),
                         Value(static_cast<int32_t>(size)), Value::SetType::AlwaysAdd);
        allocation->setProperty(eu->program()->atomizeString(ROMSTR("type")),
                         Value(ExecutionUnit::createString(type)), Value::SetType::AlwaysAdd);

        allocationsByType->setElement(eu, Value(0), Value(allocation), Value::SetType::AlwaysAdd);
    }
    
    obj->setProperty(eu->program()->atomizeString(ROMSTR("allocationsByType")),
                     Value(allocationsByType), Value::SetType::AlwaysAdd);

    eu->stack().push(Value(obj));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
