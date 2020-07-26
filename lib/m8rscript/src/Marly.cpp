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
    
    _builtinVerbs.emplace(SA::print, Value::Type::print);
    
    Scanner scanner(&stream);
    _code.reset(new List());
    
    while (true) {
        Token token = scanner.getToken();
        switch (token) {
            case Token::False:
                _code->push_back(false);
                break;
            case Token::String:
                _code->push_back(scanner.getTokenValue().str);
                break;
            case Token::Identifier: {
                Atom atom = _atomTable.atomizeString(scanner.getTokenValue().str);
                
                auto it = _builtinVerbs.find(atom);
                if (it != _builtinVerbs.end()) {
                    _code->emplace_back(it->value);
                    break;
                }
                
                auto it1 = _verbs.find(atom);
                if (it1 != _verbs.end()) {
                    _code->emplace_back(int32_t(it1 - _verbs.begin()), Value::Type::Verb);
                }
                
                // FIXME: Error
                return;
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

    for (const auto& it : *_code) {
        switch(it.type()) {
            case Value::Type::String: _stack.push(it.string(this)); break;
            case Value::Type::Verb:
                _verbs[it.integer()].value();
                break;
            default:
                // FIXME: Handle unknowns
                return;
                
            // Handle built-in verbs
            case Value::Type::print:
                _stack.pop(v);
                printf("%s", v.string(this));
                break;
        }
    }
}
