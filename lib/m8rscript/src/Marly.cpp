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

using namespace m8r;

Marly::Marly(const Stream& stream)
{
//    _verbs.emplace(SA::print, [this]()
//    {
//        Value v;
//        _stack.pop(v);
//        printf("%s", v.string(this));
//    });
    
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
                // If the identifier is of the form $<id> then push the value from
                // the _vars map with the key of <id>
                const char* id = scanner.getTokenValue().str;
                bool load = id[0] == '$';
                
                if (load) {
                    id++;
                }
                
                Atom atom = _atomTable.atomizeString(id);
                
                if (load) {
                    _codeStack.top()->emplace_back(atom.raw(), Value::Type::Load);
                    break;
                }
                
                // If the Atom ID is less than ExternalAtomOffset then
                // it is built in and there is a corresponding verb with
                // that same id
                if (atom.raw() < ExternalAtomOffset) {
                    _codeStack.top()->emplace_back(Value::Type(atom.raw()));
                    break;
                }
                
                // Try to find the id in the list of verbs
                auto it1 = _verbs.find(atom);
                if (it1 != _verbs.end()) {
                    _codeStack.top()->emplace_back(int32_t(it1 - _verbs.begin()), Value::Type::Verb);
                }
                
                // FIXME: Error
                return;
            }
            case Token::LBracket:
                _codeStack.push(SharedPtr<List>(new List()));
                break;
            case Token::RBracket:
                // When closing a list, leave it on the stack
                _stack.push(_codeStack.top().get());
                _codeStack.pop();
                break;
            case Token::At:
            case Token::Period:
            case Token::Colon: {
                // The next token must be an identifier
                Token idToken = scanner.getToken();
                if (idToken != Token::Identifier) {
                    // FIXME: Error
                    return;
                }
                
                Atom atom = _atomTable.atomizeString(scanner.getTokenValue().str);
                
                Value::Type type;
                switch (token) {
                    case Token::At:     type = Value::Type::Store; break;
                    case Token::Period: type = Value::Type::LoadProp; break;
                    case Token::Colon:  type = Value::Type::StoreProp; break;
                    default: assert(0); return;
                    
                }
                _codeStack.top()->emplace_back(atom.raw(), type);
                break;
            }
            case Token::EndOfFile:
                executeCode();
            default:
                // FIXME: Error
                return;
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
            case Value::Type::String: _stack.push(it.string()); break;
            case Value::Type::List: _stack.push(it.obj()); break;
            case Value::Type::Load: {
                auto foundValue = _vars.find(Atom(it.integer()));
                if (foundValue == _vars.end()) {
                    // FIXME: Handle error
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
                ObjectBase* obj = _stack.top().obj();
                if (!obj) {
                    // FIXME: Handle error
                    return;
                }
                _stack.pop();
                _stack.push(obj->property(Atom(it.integer())));
                break;
            }
            case Value::Type::StoreProp: {
                // Store the value in TOS-1 in the property identified by Atom(it.integer())
                // in the object on TOS
                ObjectBase* obj = _stack.top().obj();
                if (!obj) {
                    // FIXME: Handle error
                    return;
                }
                _stack.pop();
                obj->setProperty(Atom(it.integer()), _stack.top());
                _stack.pop();
                break;
            }
            case Value::Type::Verb:
                _verbs[it.integer()].value();
                break;
            default:
                // FIXME: Handle unknowns
                return;
                
            // Handle built-in verbs
            case Value::Type::print:
                _stack.pop(v);
                printf("%s", v.string());
                break;
            case Value::Type::cat: {
                SharedPtr<String> s(new String(_stack.top().string()));
                _stack.pop();
                *s += _stack.top().string();
                _stack.pop();
                _stack.push(s);
                break;
            }
        }
    }
}
