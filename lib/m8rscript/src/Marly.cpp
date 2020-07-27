/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Marly.h"

#include "Scanner.h"
#include "SystemTime.h"

using namespace m8r;

Marly::Marly(const Stream& stream, Printer printer)
    : _printer(printer)
{
    Scanner scanner(&stream);
    _codeStack.push(SharedPtr<List>(new List()));
    
    while (true) {
        Token token = scanner.getToken();
        switch (token) {
            case Token::False:
                _codeStack.top()->push_back(false);
                break;
            case Token::String:
                _codeStack.top()->push_back(scanner.getTokenValue().str);
                break;
            case Token::Integer:
                _codeStack.top()->push_back(int32_t(scanner.getTokenValue().integer));
                break;
            case Token::Identifier: {
                // If the Atom ID is less than ExternalAtomOffset then
                // it is built in and there is a corresponding verb with
                // that same id
                Atom atom = _atomTable.atomizeString(scanner.getTokenValue().str);
                if (atom.raw() < ExternalAtomOffset) {
                    _codeStack.top()->emplace_back(Value::Type(atom.raw()));
                    break;
                }
                
                // Try to find the id in the list of verbs
                auto it1 = _verbs.find(atom);
                if (it1 != _verbs.end()) {
                    _codeStack.top()->emplace_back(int32_t(it1 - _verbs.begin()), Value::Type::Verb);
                }
                
                showError(Phase::Compile, ROMString("invalid identifier"), scanner.lineno());
                return;
            }
            case Token::LBracket:
                _codeStack.push(SharedPtr<List>(new List()));
                break;
            case Token::RBracket: {
                // When closing a list, write a command to push it onto the stack
                SharedPtr<List> list = _codeStack.top();
                _codeStack.pop();
                _codeStack.top()->push_back(list);
                break;
            }
            case Token::At:
            case Token::Dollar:
            case Token::Period:
            case Token::Colon: {
                // The next token must be an identifier
                scanner.retireToken();
                Token idToken = scanner.getToken();
                if (idToken != Token::Identifier) {
                    showError(Phase::Compile, ROMString("identifier required"), scanner.lineno());
                    return;
                }
                
                Atom atom = _atomTable.atomizeString(scanner.getTokenValue().str);
                
                Value::Type type;
                switch (token) {
                    case Token::At:     type = Value::Type::Store; break;
                    case Token::Dollar: type = Value::Type::Load; break;
                    case Token::Period: type = Value::Type::LoadProp; break;
                    case Token::Colon:  type = Value::Type::StoreProp; break;
                    default: assert(0); return;
                    
                }
                _codeStack.top()->emplace_back(atom.raw(), type);
                break;
            }
            case Token::EndOfFile:
                executeCode();
                return;
            default:
                // Assume any other token is a built-in verb
                _codeStack.top()->emplace_back(int(token), Value::Type::TokenVerb);
                break;
        }
        scanner.retireToken();
    }
}

void Marly::executeCode()
{
    Value v;
    assert(_codeStack.size() == 1);

    for (const auto& it : *_codeStack.top()) {
        switch(it.type()) {
            case Value::Type::Int: _stack.push(it.integer()); break;
            case Value::Type::String: _stack.push(it.string()); break;
            case Value::Type::List: _stack.push(it.list()); break;
            case Value::Type::Load: {
                auto foundValue = _vars.find(Atom(it.integer()));
                if (foundValue == _vars.end()) {
                    showError(Phase::Run, ROMString("var not found"), 0);
                    return;
                }
                _stack.push(foundValue->value);
                break;
            }
            case Value::Type::Store:
                _vars.emplace(Atom(it.integer()), _stack.top());
                _stack.pop();
                break;
            case Value::Type::LoadProp: {
                // push the value for the property identified by Atom(it.integer())
                // of the Object on TOS
                Value val = _stack.top();
                _stack.pop();
                _stack.push(val.property(Atom(it.integer())));
                break;
            }
            case Value::Type::StoreProp: {
                // Store the value in TOS-1 in the property identified by Atom(it.integer())
                // in the object on TOS
                Value val = _stack.top();
                _stack.pop();
                val.setProperty(Atom(it.integer()), _stack.top());
                _stack.pop();
                break;
            }
            case Value::Type::Verb:
                _verbs[it.integer()].value();
                break;

            // Handle built-in verbs
            default: {
                // If the Type is < ExternalAtomOffset this is a built-in verb
                if (!it.isBuiltInVerb()) {
                    showError(Phase::Run, ROMString("unrecognized value"), 0);
                    return;
                }
                
                switch(it.builtInVerb()) {
                    case SA::print:
                        _stack.pop(v);
                        print(v.string());
                        break;
                    case SA::cat: {
                        SharedPtr<String> s2(new String(_stack.top().string()));
                        _stack.pop();
                        SharedPtr<String> s1(new String(_stack.top().string()));
                        _stack.pop();
                        *s1 += *s2;
                        _stack.push(s1);
                        break;
                    }
                    case SA::currentTime:
                        _stack.push(float(Time::now().us() / 1000000.));
                        break;
                    default: {
                        m8r::String s(ROMString("unrecognized built-in verb '"));
                        s += _atomTable.stringFromAtom(it.builtInVerb());
                        s += "'";
                        showError(Phase::Run, s.c_str(), 0);
                        return;
                    }
                }
                break;
            }
        }
    }
}

void Marly::showError(Phase phase, const char* s, uint32_t lineno) const
{
    m8r::String string = "*** ";
    string += (phase == Phase::Compile) ? "Compile" : "Runtime";
    string += " error: ";
    string += s;
    if (lineno > 0) {
        string += " on line " + m8r::String(lineno) + '\n';
    }
    string += '\n';
    print(string.c_str());
}

void Marly::print(const char* s) const
{
    if (_printer) {
        _printer(s);
    } else {
        printf("%s", s);
    }
}
