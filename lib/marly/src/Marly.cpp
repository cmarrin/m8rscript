/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Marly.h"

#include "Timer.h"
#include "SystemTime.h"

using namespace marly;

m8r::SharedPtr<m8r::Executable> MarlyScriptingLanguage::create() const
{
    return m8r::SharedPtr<m8r::Executable>(new Marly());
}

static Value timerStart(Marly* marly, const Value& value)
{
    // Params passed as a List: duration, repeat (0 or 1), list to execute, Timer
    if (value.type() != Value::Type::List || value.list()->size() != 4) {
        return Value();
    }
    
    m8r::Duration duration = m8r::Duration(value.list()->at(0).flt());
    bool repeat = value.list()->at(1).integer() != 0;
    Value body = value.list()->at(2);
    Value timerValue = value.list()->at(3);
    
    m8r::Timer* timer = reinterpret_cast<m8r::Timer*>(timerValue.property(SAtom(SA::__rawptr)).pointer());

    if (body.type() != Value::Type::List || !timer) {
        return Value();
    }
    
    timer->setCallback([marly, body](m8r::Timer*) { marly->fireEvent(body); });
    timer->start(duration, repeat ? m8r::Timer::Behavior::Repeating : m8r::Timer::Behavior::Once);
    return Value();
}

Marly::Marly()
{
    uint16_t count = 0;
    const char** list = sharedAtoms(count);
    _atomTable.setSharedAtomList(list, count);
    
    // Add global vars
    m8r::SharedPtr<Map> timer(new Map());
    timer->emplace(SAtom(SA::Once), 0);
    timer->emplace(SAtom(SA::Repeat), 1);
    timer->emplace(SAtom(SA::start), Value(timerStart));
    timer->emplace(SAtom(SA::stop), 1);
    _vars.emplace(SAtom(SA::Timer), timer);
}

bool Marly::load(const m8r::Stream& stream)
{
    _scanner.setStream(&stream);
    _codeStack.push(m8r::SharedPtr<List>(new List()));
    
    while (true) {
        m8r::Token token = _scanner.getToken();
        switch (token) {
            case m8r::Token::True:
            case m8r::Token::False:
                _codeStack.top().push_back(token == m8r::Token::True);
                break;
            case m8r::Token::String:
                _codeStack.top().push_back(_scanner.getTokenValue().str);
                break;
            case m8r::Token::Integer:
                _codeStack.top().push_back(int32_t(_scanner.getTokenValue().integer));
                break;
            case m8r::Token::Identifier: {
                // If the Atom ID is less than ExternalAtomOffset then
                // it is built in and there is a corresponding verb with
                // that same id
                m8r::Atom atom = _atomTable.atomizeString(_scanner.getTokenValue().str);
                if (atom.raw() < m8r::ExternalAtomOffset) {
                    _codeStack.top().push_back(static_cast<Value::Type>(atom.raw()));
                    break;
                }
                
                // Try to find the id in the list of verbs
                auto it1 = _verbs.find(atom);
                if (it1 != _verbs.end()) {
                    _codeStack.top().push_back(Value(int32_t(it1 - _verbs.begin()), Value::Type::Verb));
                    break;
                }
                
                if (addParseError(m8r::String::format("invalid identifier '%s'", _scanner.getTokenValue().str).c_str())) {
                    return false;
                }
                break;
            }
            case m8r::Token::LBracket:
                _codeStack.push(m8r::SharedPtr<List>(new List()));
                break;
            case m8r::Token::RBracket: {
                // When closing a list, write a command to push it onto the stack
                assert(_codeStack.top().type() == Value::Type::List);
                Value list = _codeStack.top();
                _codeStack.pop();
                _codeStack.top().push_back(list);
                break;
            }
            case m8r::Token::Dollar:    // Load var
            case m8r::Token::At:        // Store var
            case m8r::Token::Twiddle:   // Exec var
            case m8r::Token::Period:    // Load prop
            case m8r::Token::Colon:     // Store prop
            case m8r::Token::Comma:     // Exec prop
            {
                // The next token must be an identifier
                _scanner.retireToken();
                m8r::Token idToken = _scanner.getToken();
                if (idToken != m8r::Token::Identifier) {
                    if (addParseError("identifier required")) {
                        return false;
                    }
                    break;
                }
                
                m8r::Atom atom = _atomTable.atomizeString(_scanner.getTokenValue().str);
                
                Value::Type type;
                switch (token) {
                    case m8r::Token::Dollar: type = Value::Type::Load; break;
                    case m8r::Token::At:     type = Value::Type::Store; break;
                    case m8r::Token::Twiddle:type = Value::Type::Exec; break;
                    case m8r::Token::Period: type = Value::Type::LoadProp; break;
                    case m8r::Token::Colon:  type = Value::Type::StoreProp; break;
                    case m8r::Token::Comma:  type = Value::Type::ExecProp; break;
                    default: assert(0); return false;
                    
                }
                _codeStack.top().push_back(Value(atom.raw(), type));
                break;
            }
            case m8r::Token::EndOfFile:
                if (_codeStack.size() != 1) {
                    if (addParseError("misaligned code stack")) {
                        return false;
                    }
                }
                return _parseErrors.size() == 0;
            default:
                // Assume any other token is a built-in verb
                _codeStack.top().push_back(Value(int(token), Value::Type::TokenVerb));
                break;
        }
        _scanner.retireToken();
    }
}

m8r::CallReturnValue Marly::execute()
{
    enum class Action { None, Break, Continue, Return };
    Action action = Action::None;
    
    // If there is only one element on the _codeStack it is the outermost list and we
    // are just starting the program
    assert(_codeStack.size() > 0);
    if (_codeStack.size() == 1) {
        _codeStack.push(0);
        _codeStack.push(int32_t(State::Function));
    }
    assert(_codeStack.size() >= 3);

    startExec();
    
    while (true) {
        bool brk = false;
        if (action != Action::None) {
            if (action == Action::Break) {
                // break out of current loop
                if (_currentState == State::Function) {
                    _errorString = "cannot 'break' out of function";
                    return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                }
                
                brk = true;
                if (_currentState == State::LoopBody) {
                    action = Action::None;
                }
            }
        }
        
        if (brk || _currentIndex >= _currentCode->size()) {
            if (!brk && _currentState == State::LoopBody) {
                // loop again
                _currentIndex = 0;
                continue;
            }
            
            // Done with the current function. pop it
            _codeStack.pop(3);
            if (_codeStack.size() == 0) {
                return m8r::CallReturnValue(m8r::CallReturnValue::Type::Finished);
            }
            assert(_codeStack.size() >= 3);
            startExec();
            continue;
        }

        Value it = (*_currentCode)[_currentIndex++];
        
        switch(it.type()) {
            case Value::Type::Int: _stack.push(it.integer()); break;
            case Value::Type::String:
                if (it.type() == Value::Type::String) {
                    _stack.push(it.string());
                } else {
                    String s;
                    it.toString(s);
                    _stack.push(s.string().c_str());
                }
                break;
            case Value::Type::List: _stack.push(it.list()); break;
            case Value::Type::Load: {
                auto foundValue = _vars.find(m8r::Atom(it.integer()));
                if (foundValue == _vars.end()) {
                    _errorString = "var not found";
                    return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                }
                _stack.push(foundValue->value);
                break;
            }
            case Value::Type::Store:
                _vars.emplace(m8r::Atom(it.integer()), _stack.top());
                _stack.pop();
                break;
            case Value::Type::Exec: {
                auto foundValue = _vars.find(m8r::Atom(it.integer()));
                if (foundValue == _vars.end()) {
                    _errorString = "var not found";
                    return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                }
                
                if (!initExec(foundValue->value)) {
                    return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                }
                startExec();
                
                break;
            }
            case Value::Type::LoadProp: {
                // push the value for the property identified by Atom(it.integer())
                // of the Map on TOS
                Value val = _stack.top();
                _stack.pop();
                _stack.push(val.property(m8r::Atom(it.integer())));
                break;
            }
            case Value::Type::StoreProp: {
                // Store the value in TOS-1 in the property identified by Atom(it.integer())
                // in the Map on TOS
                Value val = _stack.top();
                _stack.pop();
                val.setProperty(m8r::Atom(it.integer()), _stack.top());
                _stack.pop();
                break;
            }
            case Value::Type::ExecProp: {
                // Load obj on TOS, find prop in it and exec, push returned value
                // FIXME: For now the property must be a native function. Need to
                // support List to be executed as a nested body
                Value val = _stack.top();
                _stack.pop();
                _stack.push(val.callProperty(m8r::Atom(it.integer())));
                break;
            }
            case Value::Type::Verb:
                _verbs[it.integer()].value();
                break;
                
            case Value::Type::TokenVerb: {
                switch(static_cast<m8r::Token>(it.integer())) {
                    case m8r::Token::Plus:
                    case m8r::Token::Minus:
                    case m8r::Token::Star:
                    case m8r::Token::Slash:
                    case m8r::Token::Percent: {
                        float rhs = _stack.top().flt();
                        _stack.pop();
                        float lhs = _stack.top().flt();
                        _stack.pop();
                        float result = 0;
                        switch(static_cast<m8r::Token>(it.integer())) {
                            case m8r::Token::Plus: result = lhs + rhs; break;
                            case m8r::Token::Minus: result = lhs - rhs; break;
                            case m8r::Token::Star: result = lhs * rhs; break;
                            case m8r::Token::Slash: result = lhs / rhs; break;
                            default: break;
                        }
                        _stack.push(result);
                        break;
                    }
                    default: {
                        _errorString = "unrecognized verb '";
                        _errorString += char(it.integer());
                        _errorString += "'";
                        return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                    }
                }
                break;
            }
            
            // Handle built-in verbs
            default: {
                // If the Type is < ExternalAtomOffset this is a built-in verb
                if (!it.isBuiltInVerb()) {
                    _errorString = "unrecognized value";
                    return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                }
                
                switch(it.builtInVerb()) {
                    case SA::dup:
                        _stack.push(_stack.top());
                        break;
                    case SA::swap: {
                        Value v1 = _stack.top();
                        _stack.pop();
                        Value v2 = _stack.top();
                        _stack.pop();
                        _stack.push(v1);
                        _stack.push(v2);
                        break;
                    }
                    case SA::at:
                    case SA::atput:
                    case SA::insert: {
                        int32_t i = _stack.top().integer();
                        _stack.pop();

                        Value v;
                        if (it.builtInVerb() != SA::at) {
                            v = _stack.top();
                            _stack.pop();
                        }
                        
                        // Make sure TOS is a List
                        m8r::SharedPtr<List> list = _stack.top().list();
                        _stack.pop();
                        if (!list) {
                            _errorString = "target must be List for 'insert'";
                            return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                        }
                        
                        if (it.builtInVerb() == SA::insert) {
                            if (i > list->size()) {
                                i = int32_t(list->size());
                            }
                            list->insert(list->begin() + i, v);
                        } else {
                            if (i >= list->size()) {
                                _errorString = m8r::String::format("at index %d out of range for list of size %d", i, list->size());
                                return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                             }
                        
                            if (it.builtInVerb() == SA::at) {
                                _stack.push((*list)[i]);
                            } else {
                                (*list)[i] = v;
                            }
                        }
                        break;
                    }
                    case SA::lt:
                    case SA::le:
                    case SA::eq:
                    case SA::ne:
                    case SA::ge:
                    case SA::gt: {
                        float rhs = _stack.top().flt();
                        _stack.pop();
                        float lhs = _stack.top().flt();
                        _stack.pop();
                        bool result = false;
                        switch(it.builtInVerb()) {
                            case SA::lt: result = lhs < rhs; break;
                            case SA::le: result = lhs <= rhs; break;
                            case SA::eq: result = lhs == rhs; break;
                            case SA::ne: result = lhs != rhs; break;
                            case SA::ge: result = lhs >= rhs; break;
                            case SA::gt: result = lhs > rhs; break;
                            default: break;
                        }
                        _stack.push(result);
                        break;
                    }
                    case SA::inc:
                    case SA::dec: {
                        if (_stack.top().type() == Value::Type::Int) {
                            int32_t i = _stack.top().integer();
                            if (it.builtInVerb() == SA::inc) {
                                i++;
                            } else {
                                i--;
                            }
                            _stack.top() = i;
                        } else {
                             float f = _stack.top().flt();
                            if (it.builtInVerb() == SA::inc) {
                                f = f + 1;
                            } else {
                                f = f + 1;
                            }
                            _stack.top() = f;
                       }
                       break;
                    }
                    case SA::println:
                    case SA::print: {
                        String s;
                        _stack.top().toString(s);
                        print(s.string().c_str());
                        _stack.pop();
                        if (it.builtInVerb() == SA::println) {
                            print("\n");
                        }
                        break;
                    }
                    case SA::cat: {
                        String s1, s2;
                        _stack.top().toString(s2);
                        _stack.pop();
                        _stack.top().toString(s1);
                        _stack.pop();
                        s1.string() += s2.string();
                        _stack.push(s1.string().c_str());
                        break;
                    }
                    case SA::currentTime: {
                        float t = float(double(m8r::Time::now().us()) / 1000000);
                        _stack.push(t);
                        break;
                    }
                    case SA::delay:
                        startDelay(m8r::Duration(_stack.top().flt()));
                        _stack.pop();
                        _codeStack.top(-1) = Value(_currentIndex);
                        return m8r::CallReturnValue(m8r::CallReturnValue::Type::Delay);
                    case SA::new$: {
                        // Create a new Map and call the __ctor of the Value in TOS
                        Value obj = _stack.top();
                        _stack.pop();
                        
                        m8r::SharedPtr<Map> map(new Map(obj));
                        break;
                    }
                    case SA::loop:
                        if (!initExec(_stack.top(), State::LoopBody)) {
                            return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                        }
                        _stack.pop();
                        startExec();
                        break;
                    case SA::break$:
                        action = Action::Break;
                        break;
                    case SA::if$:
                        // Stack has body and bool. If bool is true execute body
                        if (_stack.top(-1).boolean()) {
                            if (!initExec(_stack.top(), State::Body)) {
                                return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                            }
                            _stack.pop(2);
                            startExec();
                        } else {
                            _stack.pop(2);
                        }
                        break;
//                    case SA::while {
//                        // Push the while body and test
//                        _codeStack.push(_stack.top()); // body
//                        _codeStack.push(_stack.top(-1)); // test
//                        _stack.pop(2);
//                        _currentState = State::WhileTest;
//                        
//                        if (!initExec(_codeStack.top(), State::WhileTest) {
//                            return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
//                        }
//                        startExec();
//                    }
//                    case SA::for$: {
//                        m8r::SharedPtr<List> body = _stack.top().list();
//                        _stack.pop();
//                        m8r::SharedPtr<List> iter = _stack.top().list();
//                        _stack.pop();
//                        m8r::SharedPtr<List> test = _stack.top().list();
//                        _stack.pop();
//                        
//                        // Iteration value is on TOS.
//                        // FIXME: Check that the stack is the same size before executing each list
//                        Value testResult;
//                        
//                        while (true) {
//                            if (!execute(test)) {
//                                return false;
//                            }
//                            _stack.pop(testResult);
//                            if (!testResult.boolean()) {
//                                _stack.pop();
//                                break;
//                            }
//                            if (!execute(body)) {
//                                return false;
//                            }
//                            if (!execute(iter)) {
//                                return false;
//                            }
//                        }
//                        break;
//                    }
                    default: {
                        _errorString = m8r::String::format("unrecognized built-in verb '%s'", 
                                        _atomTable.stringFromAtom(m8r::Atom(static_cast<m8r::Atom::value_type>(it.builtInVerb()))));
                        return m8r::CallReturnValue(m8r::Error::Code::RuntimeError);
                    }
                }
                break;
            }
        }
    }
}

bool Marly::initExec(const Value& list, State state)
{
    if (list.type() != Value::Type::List) {
        _errorString = "value to exec must be List";
        return false;
    }
    
    // Save the current index and state
    _codeStack.top() = Value(int32_t(_currentState));
    _codeStack.top(-1) = Value(_currentIndex);

    _codeStack.push(list);
    _codeStack.push(0);
    _codeStack.push(int32_t(state));
    return true;
}

void Marly::startExec()
{
    _currentState = static_cast<State>(_codeStack.top().integer());
    _currentIndex = _codeStack.top(-1).integer();
    _currentCode = _codeStack.top(-2).list();
    assert(_currentIndex >= 0 && _currentIndex <= _currentCode->size());
}
