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

CallReturnValue FSProto::unmount(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    system()->fileSystem()->unmount();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::format(ExecutionUnit*, Value thisValue, uint32_t nparams)
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

CallReturnValue FSProto::openDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::makeDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::remove(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::rename(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::stat(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::lastError(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FSProto::errorString(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

FileProto::FileProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::File, parent)
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

CallReturnValue FileProto::close(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::read(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::write(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::seek(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::tell(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::eof(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::valid(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::error(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::type(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

DirectoryProto::DirectoryProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::Directory, parent)
{
    addProperty(program, SA::name, name);
    addProperty(program, SA::size, size);
    addProperty(program, SA::type, type);
    addProperty(program, SA::valid, valid);
    addProperty(program, SA::error, error);
    addProperty(program, SA::next, next);
}

CallReturnValue DirectoryProto::name(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::size(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::type(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::valid(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::error(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue DirectoryProto::next(ExecutionUnit*, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
