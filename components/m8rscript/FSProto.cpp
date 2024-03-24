/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "TCPProto.h"

#include "ExecutionUnit.h"
#include "MFS.h"
#include "SystemInterface.h"
#include "TCP.h"

using namespace m8rscript;
using namespace m8r;

static StaticObject::StaticFunctionProperty _propsFS[] =
{
    { SA::mount, FSProto::mount },
    { SA::mounted, FSProto::mounted },
    { SA::unmount, FSProto::unmount },
    { SA::format, FSProto::format },
    { SA::open, FSProto::open },
    { SA::openDirectory, FSProto::openDirectory },
    { SA::makeDirectory, FSProto::makeDirectory },
    { SA::remove, FSProto::remove },
    { SA::rename, FSProto::rename },
    { SA::stat, FSProto::stat },
    { SA::lastError, FSProto::lastError },
    { SA::errorString, FSProto::errorString },
};

FSProto::FSProto()
{
    setProperties(_propsFS, sizeof(_propsFS) / sizeof(StaticFunctionProperty));
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
    Mad<Object> obj = ObjectFactory::create(SAtom(SA::File), eu, nparams);
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::Unimplemented);
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

static StaticObject::StaticFunctionProperty _propsFile[] =
{
    { SA::constructor, FileProto::constructor },
    { SA::close, FileProto::close },
    { SA::read, FileProto::read },
    { SA::write, FileProto::write },
    { SA::seek, FileProto::seek },
    { SA::eof, FileProto::eof },
    { SA::valid, FileProto::valid },
    { SA::error, FileProto::error },
    { SA::type, FileProto::type },
};

FileProto::FileProto()
{
    setProperties(_propsFile, sizeof(_propsFile) / sizeof(StaticFunctionProperty));
}

CallReturnValue FileProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // Params are a filename string and openmode string. If openmode is missing, defaults to "r"
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::MissingThis);
    }

    if (nparams < 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    String filename = eu->stack().top(1 - nparams).toStringValue(eu);
    if (filename.empty()) {
        return CallReturnValue(Error::Code::InvalidArgumentValue);
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
        return CallReturnValue(Error::Code::InvalidArgumentValue);
    }
        
    Mad<File> file = system()->fileSystem()->open(filename.c_str(), mode);
    
    obj->setProperty(SAtom(SA::__impl), Value(file.get()), Value::SetType::AlwaysAdd);

    // Add a destructor for the file
    Value dtor([](ExecutionUnit*, Value thisValue, uint32_t nparams)
    {
        File* file = reinterpret_cast<File*>(thisValue.property(SAtom(SA::__impl)).asRawPointer());
        if (file) {
            delete file;
            thisValue.setProperty(SAtom(SA::__impl), Value(), Value::SetType::AddIfNeeded);
        }
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    });
    
    thisValue.setProperty(SAtom(SA::__destructor), dtor, Value::SetType::AddIfNeeded);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::close(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    SharedPtr<File> file = thisValue.isObject() ? thisValue.asObject()->impl<File>() : SharedPtr<File>();
    if (!file) {
        return CallReturnValue(Error::Code::InternalError);
    }

    file->close();
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue FileProto::read(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    SharedPtr<File> file = thisValue.isObject() ? thisValue.asObject()->impl<File>() : SharedPtr<File>();
    if (!file) {
        return CallReturnValue(Error::Code::InternalError);
    }

    // Params: size ==> return String with data
    // TODO: How do we do binary?
    if (nparams != 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    int32_t size = eu->stack().top(1 - nparams).toIntValue(eu);
    if (size <= 0) {
        return CallReturnValue(Error::Code::InvalidArgumentValue);
    }
    
    Mad<char> buf = Mad<char>::create(size);
    file->read(buf.get(), size);
    eu->stack().push(Value(ExecutionUnit::createString(buf.get(), size)));
    buf.destroy();
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
    SharedPtr<File> file = thisValue.isObject() ? thisValue.asObject()->impl<File>() : SharedPtr<File>();
    if (!file) {
        return CallReturnValue(Error::Code::InternalError);
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

static StaticObject::StaticFunctionProperty _propsDirectory[] =
{
    { SA::constructor, DirectoryProto::constructor },
    { SA::name, DirectoryProto::name },
    { SA::size, DirectoryProto::size },
    { SA::type, DirectoryProto::type },
    { SA::valid, DirectoryProto::valid },
    { SA::error, DirectoryProto::error },
    { SA::next, DirectoryProto::next },
};

DirectoryProto::DirectoryProto()
{
    setProperties(_propsDirectory, sizeof(_propsDirectory) / sizeof(StaticFunctionProperty));
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

static bool makePathString(m8r::String& fullPath, const m8r::String& filename, const m8r::String& path, const m8r::String& home, const m8r::String& cwd)
{
    if (home.front() != '/' || cwd.front() != '/') {
        return false;
    }
    
    fullPath = path;

    if (path.back() != '/') {
        fullPath += '/';
    }
    fullPath += filename;
    
    if (fullPath.front() == '~') {
        fullPath = fullPath.slice(1);
        if (home.back() != '/') {
            fullPath = home + "/" + fullPath;
        } else {
            fullPath = home + fullPath;
        }
    } else if (fullPath.front() == '.') {
        fullPath = fullPath.slice(1);
        if (cwd.back() != '/') {
            fullPath = cwd + "/" + fullPath;
        } else {
            fullPath = cwd + fullPath;
        }
    }
    return true;
}

m8r::String FSProto::findPath(ExecutionUnit* eu, const m8r::String& filename, const Mad<Object>& env)
{
    String fullPath;
    Value paths = env->property(eu->program()->atomizeString("PATH"));
    String home = env->property(eu->program()->atomizeString("HOME")).toStringValue(eu);
    String cwd = env->property(eu->program()->atomizeString("CWD")).toStringValue(eu);
    
    int32_t size = paths.property(SAtom(SA::length)).toIntValue(eu);
    for (int32_t i = 0; i < size; ++i) {
        String path = paths.element(eu, Value(i)).toStringValue(eu);
        if (makePathString(fullPath, filename, path, home, cwd)) {
            if (system()->fileSystem()->exists(fullPath.c_str())) {
                return fullPath;
            }
        }
    }
    return String();
}
