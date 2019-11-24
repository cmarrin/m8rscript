/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Task.h"

#include "Application.h"
#include "MStream.h"

#ifndef NO_PARSER_SUPPORT
#include "Parser.h"
#endif

#ifdef PRINT_CODE
#include "CodePrinter.h"
#endif

#include <cassert>

using namespace m8r;

void TaskBase::finish()
{
    if (_finishCB) {
        _finishCB(this);
    }
}

Task::Task()
{
    _eu = Mad<ExecutionUnit>::create();
    GC::addEU(_eu.raw());
}    

Task::~Task()
{
    GC::removeEU(_eu.raw());
    _eu.destroy();
}

void Task::setFilename(const char* filename)
{
    // FIXME: What do we do with these?
    ErrorList syntaxErrors;
    Parser::Debug debug = Parser::Debug::Full;
    
    if (filename) {
        FileStream m8rStream(system()->fileSystem(), filename);
        if (!m8rStream.loaded()) {
            _error = Error::Code::FileNotFound;
            return;
        }
        
#ifdef NO_PARSER_SUPPORT
        return;
#else
        // See if we can parse it
        system()->lock();
        Parser parser;
        parser.parse(m8rStream, _eu.get(), debug);
        system()->unlock();
        if (parser.nerrors()) {
            _eu->printf(ROMSTR("***** %d error%s parsing %s\n\n"), parser.nerrors(), (parser.nerrors() == 1) ? "" : "s", filename);
            syntaxErrors.swap(parser.syntaxErrors());
            _error = Error::Code::ParseError;
            return;
        } else {
#ifdef PRINT_CODE
            CodePrinter codePrinter;
            m8r::String codeString = codePrinter.generateCodeString(parser.program());
            
            system()->printf(ROMSTR("\n*** Start Generated Code ***\n\n"));
            system()->printf("%s", codeString.c_str());
            system()->printf(ROMSTR("\n*** End of Generated Code ***\n\n"));
#endif
        }
        
        _eu->startExecution(parser.program());
#endif
    }
}

void Task::receivedData(const String& data, Telnet::Action action)
{
    _eu->receivedData(data, action);
}

void Task::setConsolePrintFunction(std::function<void(const String&)> f)
{
    _eu->setConsolePrintFunction(f);
}

void Task::setConsoleListener(Value func)
{
    _eu->setConsoleListener(func);
}

CallReturnValue Task::execute()
{
    return _eu->continueExecution();
}

TaskProto::TaskProto()
: StaticObject({
    { SA::constructor, Value(constructor) },
    { SA::run, Value(run) },
})
{ }

CallReturnValue TaskProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // filename, consoleListener
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Value param = eu->stack().top(1 - nparams);
    String filename;
    if (param.isString()) {
        filename = param.toStringValue(eu);
    } else {
        return CallReturnValue(CallReturnValue::Error::InvalidArgumentValue);
    }
    
    Value consoleListener;
    if (nparams > 1) {
        consoleListener = eu->stack().top(2 - nparams);
    }
    
    Mad<Task> task = Mad<Task>::create();
    task->setConsolePrintFunction(eu->consolePrintFunction());
    task->setFilename(filename.c_str());
    
    if (task->error() != Error::Code::OK) {
        Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("unable to load task '%s'"), filename.c_str());;
        return CallReturnValue(CallReturnValue::Error::Error);
    }
    obj->setProperty(Atom(SA::__nativeObject), Value::asValue(task), Value::SetPropertyType::AlwaysAdd);
    
    obj->setProperty(Atom(SA::arguments), Value::asValue(task), Value::SetPropertyType::AlwaysAdd);
    obj->setProperty(Atom(SA::env), Value::asValue(task), Value::SetPropertyType::AlwaysAdd);
    
    task->setConsoleListener(consoleListener);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TaskProto::run(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams > 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Mad<Task> task;
    CallReturnValue retval = getNative(task, eu, thisValue);
    if (retval.error() != CallReturnValue::Error::Ok) {
        return retval;
    }
    
    Value func;
    
    if (nparams > 0) {
        func = eu->stack().top(1 - nparams);
    }
    
    // Store func so it doesn't get gc'ed
    thisValue.setProperty(eu, Atom(SA::__object), func, Value::SetPropertyType::AddIfNeeded);

    task->run([eu, func](TaskBase* task)
    {
        if (func) {
            Value arg(static_cast<int32_t>(task->error().code()));
            eu->fireEvent(func, Value(), &arg, 1);
        }
    });

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
