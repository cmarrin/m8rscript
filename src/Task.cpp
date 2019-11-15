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
    GC::addEU(_eu);
}    

Task::~Task()
{
    GC::removeEU(_eu);
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
        system()->printf(ROMSTR("Parsing...\n"));
        system()->lock();
        Parser parser;
        parser.parse(m8rStream, debug);
        system()->unlock();
        system()->printf(ROMSTR("Finished parsing %s. %d error%s\n\n"), filename, parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
        if (parser.nerrors()) {
            syntaxErrors.swap(parser.syntaxErrors());
            _error = Error::Code::ParseError;
            return;
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

TaskProto::TaskProto(Mad<Program> program, ObjectFactory* parent)
    : ObjectFactory(program, SA::Task, parent, constructor)
{
    addProperty(program, SA::run, run);

    _obj->setArray(true);
    _obj->resize(4);
    (*_obj)[0] = Value(0);
    (*_obj)[1] = Value(0);
    (*_obj)[2] = Value(0);
    (*_obj)[3] = Value(0);
}

CallReturnValue TaskProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // filename, consoleListener
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Mad<Object> obj = thisValue.asObject();
    if (!obj) {
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
    task->setFilename(filename.c_str());
    
    if (task->error() != Error::Code::OK) {
        Error::printError(Error::Code::RuntimeError, eu->lineno(), ROMSTR("unable to load task '%s'"), filename.c_str());;
        return CallReturnValue(CallReturnValue::Error::Error);
    }
    obj->setProperty(eu, Atom(SA::__nativeObject), Value::asValue(task), Value::SetPropertyType::AlwaysAdd);
    
    obj->setProperty(eu, Atom(SA::arguments), Value::asValue(task), Value::SetPropertyType::AlwaysAdd);
    obj->setProperty(eu, Atom(SA::env), Value::asValue(task), Value::SetPropertyType::AlwaysAdd);
    
    // Set the console funcs
    task->setConsoleListener(consoleListener);
    task->setConsolePrintFunction(eu->consolePrintFunction());

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

    task->run([eu, task, thisValue](TaskBase*)
    {
        Value func = thisValue.property(eu, Atom(SA::__object));
        if (func) {
            Value arg(static_cast<int32_t>(task->error().code()));
            eu->fireEvent(func, Value::asValue(task), &arg, 1);
        }
    });

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
