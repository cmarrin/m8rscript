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

Task::Task(const char* filename)
{
    _eu = new ExecutionUnit();
    Object::addEU(_eu);
    
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

Task::~Task()
{
    Object::removeEU(_eu);
    delete _eu;
}

void Task::receivedData(const String& data, Telnet::Action action)
{
    _eu->receivedData(data, action);
}

void Task::setConsolePrintFunction(std::function<void(const String&)> f)
{
    _eu->setConsolePrintFunction(f);
}

CallReturnValue Task::execute()
{
    return _eu->continueExecution();
}

TaskProto::TaskProto(Program* program, ObjectFactory* parent)
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
    // filename
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Object* obj = thisValue.asObject();
    if (!obj) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    String filename = eu->stack().top(1 - nparams).toStringValue(eu);
    Task* task = new Task(filename.c_str());
    obj->setProperty(eu, Atom(SA::__nativeObject), Value(task), Value::SetPropertyType::AlwaysAdd);

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TaskProto::run(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}
