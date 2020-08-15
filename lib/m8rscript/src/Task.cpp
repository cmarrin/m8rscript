/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Task.h"

#include "GC.h"
#include "FileStream.h"
#if LUA_SUPPORT == 1
#include "LuaEngine.h"
#endif
#if MARLY_SUPPORT == 1
#include "Marly.h"
#endif
#if M8RSCRIPT_SUPPORT == 1
#include "Parser.h"
#endif
#include "SystemInterface.h"

#ifndef NDEBUG
#ifdef __APPLE__
//#define PRINT_CODE
#endif
#endif

#ifdef PRINT_CODE
#include "CodePrinter.h"
#endif

#include <cassert>

using namespace m8r;

void Task::print(const char* s) const
{
    if (_executable) {
        _executable->print(s);
    } else {
        system()->print(s);
    }
}

Task::~Task()
{
    if (_executable) {
#if M8RSCRIPT_SUPPORT == 1
        Mad<ExecutionUnit> eu = Mad<ExecutionUnit>(reinterpret_cast<ExecutionUnit*>(_executable.get()));
        GC::removeEU(eu.raw());
        _executable.reset();
        GC::gc();
#else
        _executable.reset();
#endif
    }
}

bool Task::load(const char* filename)
{
    Vector<String> parts = String(filename).split(".");
    if (parts.size() < 2) {
        // Try appending known suffixes
        String baseFilename = filename;

#if M8RSCRIPT_SUPPORT == 1
        if (load((baseFilename + ".m8r").c_str())) {
            return true;
        }
#endif

#if MARLY_SUPPORT == 1
        if (load((baseFilename + ".marly").c_str())) {
            return true;
        }
#endif

#if LUA_SUPPORT == 1
         if (load((baseFilename + ".lua").c_str())) {
            return true;
        }
#endif
         
        system()->printf("***** Missing suffix for '%s'\n\n", filename);
        return false;
    }
    
    Error error(Error::Code::NoFS);
    Mad<File> file;

    if (system()->fileSystem()) {
        file = system()->fileSystem()->open(filename, FS::FileOpenMode::Read);
        error = file->error();
    }

    if (error) {
        print(Error::formatError(error.code(), "Unable to open '%s' for execution", filename).c_str());
        _error = error;
        return false;
    }
    
    bool ret = load(FileStream(file), parts.back());

    if (file->error() != Error::Code::OK) {
        print(Error::formatError(file->error().code(), "Error reading '%s'", filename).c_str());
    }
        
    file.destroy(MemoryType::Native);
    return ret;
}

bool Task::load(const Stream& stream, const String& type)
{
#ifndef NDEBUG
    _name = String::format("Task(%p)", this);
#endif

    if (type == "m8r") {
#if M8RSCRIPT_SUPPORT == 1
        std::shared_ptr<ExecutionUnit> eu = std::make_shared<ExecutionUnit>();
        _executable = eu;
        GC::addEU(Mad<ExecutionUnit>(eu.get()).raw());
        
        // See if we can parse it
        ParseErrorList errorList;
        Parser parser;
        parser.parse(stream, eu.get(), Parser::debug);
        if (parser.nerrors()) {
            _executable->printf("***** %d parse error%s\n\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
            errorList.swap(parser.syntaxErrors());
            _error = Error::Code::ParseError;
            return false;
        }
        
        eu->startExecution(parser.program());
        #ifdef PRINT_CODE
        if (!parser.nerrors()) {
            CodePrinter codePrinter;
            m8r::String codeString = codePrinter.generateCodeString(eu.get());
            
            system()->printf("\n*** Start Generated Code ***\n\n");
            system()->printf("%s", codeString.c_str());
            system()->printf("\n*** End of Generated Code ***\n\n");
        }
        #endif
        return true;
#endif
    } else if (type == "marly") {
#if MARLY_SUPPORT == 1
        Marly marly(stream, [this](const char* s) { print(s); });
        return true;
#endif
    } else if (type == "lua") {
#if LUA_SUPPORT == 1
        std::shared_ptr<LuaEngine> engine = std::make_shared<LuaEngine>(stream);
        if (engine->nerrors() > 0) {
            engine->printf("***** %d lua error%s\n\n", engine->nerrors(), (engine->nerrors() == 1) ? "" : "s");
        } else {
            _executable = engine;
        }
#endif
    } else {
        system()->printf("***** Unknown suffix '%s'.\n\n", type.c_str());
    }

    return true;
}

#if M8RSCRIPT_SUPPORT == 1
static StaticObject::StaticFunctionProperty _functionProps[] =
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
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    String filename;
    if (nparams > 0) {
        Value param = eu->stack().top(1 - nparams);
        if (param.isString()) {
            filename = param.toStringValue(eu);
        } else {
            return CallReturnValue(CallReturnValue::Error::InvalidArgumentValue);
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
        return CallReturnValue(CallReturnValue::Error::Error);
    }
    
    task->setConsolePrintFunction(eu->consolePrintFunction());

    obj->setNativeObject(task);
    obj->setProperty(Atom(SA::arguments), Value::NullValue(), Value::SetType::AlwaysAdd);
    obj->setProperty(Atom(SA::env), envValue, Value::SetType::AlwaysAdd);
    
    eu->setConsoleListener(consoleListener);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TaskProto::run(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    //      1) run(function)
    
    if (nparams > 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    std::shared_ptr<Task> task = thisValue.isObject() ? thisValue.asObject()->nativeObject<Task>() : nullptr;
    if (!task) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }

    Value func;
    if (nparams == 1) {
        func = eu->stack().top(1 - nparams);
    }
    
    // Store func so it doesn't get gc'ed
    thisValue.setProperty(eu, Atom(SA::__object), func, Value::SetType::AddIfNeeded);
    
    system()->taskManager()->run(task, [eu, func](Task* task)
    {
        if (func) {
            Value arg(static_cast<int32_t>(task->error().code()));
            eu->fireEvent(func, Value(), &arg, 1);
        }
    });

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
#endif
