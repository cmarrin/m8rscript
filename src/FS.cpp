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

FSProto::FSProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::FS, parent)
{
    addProperty(program, SA::mount, mount);
    addProperty(program, SA::mounted, mounted);
    addProperty(program, SA::unmount, unmount);
    addProperty(program, SA::format, format);
    addProperty(program, SA::open, open);
    addProperty(program, SA::openDirectory, openDirectory);
    addProperty(program, SA::makeDirectory, makeDirectory);
    addProperty(program, SA::remove, remove);
    addProperty(program, SA::rename, rename);
    addProperty(program, SA::stat, stat);
    addProperty(program, SA::lastError, lastError);
    addProperty(program, SA::errorString, errorString);
}

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
    Object* obj = ObjectFactory::create(Atom(SA::File), eu, nparams);
    if (!obj) {
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

FileProto::FileProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::File, parent, constructor)
{
    addProperty(program, SA::close, close);
    addProperty(program, SA::read, read);
    addProperty(program, SA::write, write);
    addProperty(program, SA::seek, seek);
    addProperty(program, SA::eof, eof);
    addProperty(program, SA::valid, valid);
    addProperty(program, SA::error, error);
    addProperty(program, SA::type, type);
}

CallReturnValue FileProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // Params are a filename string and openmode string. If openmode is missing, defaults to "r"
    Object* obj = thisValue.asObject();
    if (!obj) {
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
        
    File* file = system()->fileSystem()->open(filename.c_str(), mode);
    
    // TODO: This certainly won't work
    obj->setProperty(eu, Atom(SA::__nativeObject), Value(file), Value::SetPropertyType::AlwaysAdd);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::close(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    File* file;
    CallReturnValue ret = getNative(file, eu, thisValue);
    if (ret.error() != CallReturnValue::Error::Ok) {
        return ret;
    }
    
    file->close();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::read(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    File* file;
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
    
    char* buf = new char[size];
    file->read(buf, size);
    eu->stack().push(Value(eu->program()->createString(buf, size)));
    delete [ ] buf;
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
    File* file;
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

DirectoryProto::DirectoryProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::Directory, parent, constructor)
{
    addProperty(program, SA::name, name);
    addProperty(program, SA::size, size);
    addProperty(program, SA::type, type);
    addProperty(program, SA::valid, valid);
    addProperty(program, SA::error, error);
    addProperty(program, SA::next, next);
}

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
