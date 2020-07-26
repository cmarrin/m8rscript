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
            case Token::Identifier: {
                Atom atom = _atomTable.atomizeString(scanner.getTokenValue().str);
                
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
