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
#include "SystemInterface.h"
#include "TCP.h"

using namespace m8r;

static m8r::StaticObject::StaticFunctionProperty _functionProps[] =
{
    { SA::constructor, TaskProto::constructor },
    { SA::run, TaskProto::run },
};

TaskProto::TaskProto()
{
    setProperties(_functionProps, sizeof(_functionProps) / sizeof(StaticFunctionProperty));
}

CallReturnValue TaskProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // filename, consoleListener, <env>
    //
    // If env is present it is an object with:
    //
    //      CWD     - current working directory
    //      PATH    - Array of path prefix strings
    //      HOME    - Path to home directory
    //
    // If env is not passed, the filename is taken as an absolute path. If it doesn't start
    // with '/' it will fail. If env is present then the following rules are applied:
    //
    // For each entry in PATH:
    //
    //      1) Prepend filename with the entry
    //      2) If the leading character of the result is '.', replace it with CWD.
    //      3) If the leading character of the resiult is '~', replace it with HOME.
    //      4) Search for a file with that name.
    //
    //  Notes:  At each prepending step if the prefix does not end with '/', add one
    //          CWD and HOME must be absolute paths or an error is returned
    //
    // Task can also be called with 0 params, in which case it simply initializes without
    // a command. Used so you can call run with a function and timeout value to act as
    // a timer.
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(Error::Code::MissingThis);
    }
    
    String filename;
    if (nparams > 0) {
        Value param = eu->stack().top(1 - nparams);
        if (param.isString()) {
            filename = param.toStringValue(eu);
        } else {
            return CallReturnValue(Error::Code::InvalidArgumentValue);
        }
    }
    
    Value consoleListener;
    if (nparams > 1) {
        consoleListener = eu->stack().top(2 - nparams);
    }
    
    Value envValue;
    if (nparams > 2) {
        envValue = eu->stack().top(3 - nparams);
    }
    
    Mad<Object> env = envValue.asObject();
    
    String path;
    if (!filename.empty()) {
        path = FSProto::findPath(eu, filename, env);
    }
    
    std::shared_ptr<Task> task = std::make_shared<Task>();

    if (!filename.empty()) {
        task->load(path.c_str());
    }
    
    if (task->error() != Error::Code::OK) {
        eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(),
                                     "unable to load task '%s'", filename.c_str()).c_str());
        return CallReturnValue(task->error());
    }
    
    task->setConsolePrintFunction(eu->consolePrintFunction());

    obj->setNativeObject(task);
    obj->setProperty(SAtom(SA::arguments), Value::NullValue(), Value::SetType::AlwaysAdd);
    obj->setProperty(SAtom(SA::env), envValue, Value::SetType::AlwaysAdd);
    
    eu->setConsoleListener(consoleListener);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TaskProto::run(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    //      1) run(function)
    
    if (nparams > 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    std::shared_ptr<Task> task = thisValue.isObject() ? thisValue.asObject()->nativeObject<Task>() : nullptr;
    if (!task) {
        return CallReturnValue(Error::Code::InternalError);
    }

    Value func;
    if (nparams == 1) {
        func = eu->stack().top(1 - nparams);
    }
    
    // Store func so it doesn't get gc'ed
    thisValue.setProperty(SAtom(SA::__object), func, Value::SetType::AddIfNeeded);
    
    system()->taskManager()->run(task, [eu, func](Task* task)
    {
        if (func) {
            Value arg(static_cast<int32_t>(task->error().code()));
            eu->fireEvent(func, Value(), &arg, 1);
        }
    });

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
