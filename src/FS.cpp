/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "FS.h"

#include "ExecutionUnit.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

FSProto::FSProto()
    : StaticObject({
        { SA::mount, Value(mount) },
        { SA::mounted, Value(mounted) },
        { SA::unmount, Value(unmount) },
        { SA::format, Value(format) },
        { SA::open, Value(open) },
        { SA::openDirectory, Value(openDirectory) },
        { SA::makeDirectory, Value(makeDirectory) },
        { SA::remove, Value(remove) },
        { SA::rename, Value(rename) },
        { SA::stat, Value(stat) },
        { SA::lastError, Value(lastError) },
        { SA::errorString, Value(errorString) },
    })
{ }

CallReturnValue FSProto::mount(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    system()->fileSystem()->mount();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::mounted(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    eu->stack().push(Value(system()->fileSystem()->mounted()));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue FSProto::unmount(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    system()->fileSystem()->unmount();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::format(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    system()->fileSystem()->format();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::open(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<Object> obj = ObjectFactory::create(Atom(SA::File), eu, nparams);
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::Unimplemented);
    }
    
    eu->stack().push(Value(obj));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue FSProto::openDirectory(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::makeDirectory(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::remove(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::rename(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::stat(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::lastError(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::errorString(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

FileProto::FileProto()
    : StaticObject({
        { SA::constructor, Value(constructor) },
        { SA::close, Value(close) },
        { SA::read, Value(read) },
        { SA::write, Value(write) },
        { SA::seek, Value(seek) },
        { SA::eof, Value(eof) },
        { SA::valid, Value(valid) },
        { SA::error, Value(error) },
        { SA::type, Value(type) },
    })
{ }

CallReturnValue FileProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // Params are a filename string and openmode string. If openmode is missing, defaults to "r"
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }

    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    String filename = eu->stack().top(1 - nparams).toStringValue(eu);
    if (filename.empty()) {
        return CallReturnValue(CallReturnValue::Error::InvalidArgumentValue);
    }
    
    String modeString;
    if (nparams > 1) {
        modeString = eu->stack().top(2 - nparams).toStringValue(eu);
    }
    
    FS::FileOpenMode mode;
    if (modeString.empty() || modeString == "r") {
        mode = FS::FileOpenMode::Read;
    } else if (modeString == "r+") {
        mode = FS::FileOpenMode::ReadUpdate;
    } else if (modeString == "w") {
        mode = FS::FileOpenMode::Write;
    } else if (modeString == "w+") {
        mode = FS::FileOpenMode::WriteUpdate;
    } else if (modeString == "a") {
        mode = FS::FileOpenMode::Append;
    } else if (modeString == "a+") {
        mode = FS::FileOpenMode::AppendUpdate;
    } else if (modeString == "c") {
        mode = FS::FileOpenMode::Create;
    } else {
        return CallReturnValue(CallReturnValue::Error::InvalidArgumentValue);
    }
        
    Mad<File> file = system()->fileSystem()->open(filename.c_str(), mode);
    
    obj->setProperty(Atom(SA::__nativeObject), Value::asValue(file), Value::SetPropertyType::AlwaysAdd);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::close(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<File> file;
    CallReturnValue ret = getNative(file, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
    }
    
    file->close();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::read(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<File> file;
    CallReturnValue ret = getNative(file, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
    }
    
    // Params: size ==> return String with data
    // TODO: How do we do binary?
    if (nparams != 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    int32_t size = eu->stack().top(1 - nparams).toIntValue(eu);
    if (size <= 0) {
        return CallReturnValue(CallReturnValue::Error::InvalidArgumentValue);
    }
    
    Mad<char> buf = Mad<char>::create(size);
    file->read(buf.get(), size);
    eu->stack().push(Value(Mad<String>::create(buf.get(), size)));
    buf.destroy(size);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue FileProto::write(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::seek(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::tell(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::eof(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::valid(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    Mad<File> file;
    CallReturnValue ret = getNative(file, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
    }
    eu->stack().push(Value(file->valid()));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue FileProto::error(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::type(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

DirectoryProto::DirectoryProto()
    : StaticObject({
        { SA::constructor, Value(constructor) },
        { SA::name, Value(name) },
        { SA::size, Value(size) },
        { SA::type, Value(type) },
        { SA::valid, Value(valid) },
        { SA::error, Value(error) },
        { SA::next, Value(next) },
    })
{ }

CallReturnValue DirectoryProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::name(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::size(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::type(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::valid(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::error(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::next(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
