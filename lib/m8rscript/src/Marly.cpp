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
    _nativeVerbs.emplace(SA::print, [this]()
    {
        Value v;
        _stack.pop(v);
        printf("%s", v.string(this));
    });
    
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
                auto it = _nativeVerbs.find(_atomTable.atomizeString(scanner.getTokenValue().str));
                if (it == _nativeVerbs.end()) {
                    // FIXME: Error
                    return;
                }
                _code->emplace_back(int32_t(it - _nativeVerbs.begin()), Value::Type::NativeVerb);
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
    for (const auto& it : *_code) {
        switch(it.type()) {
            case Value::Type::String: _stack.push(it.string(this)); break;
            case Value::Type::NativeVerb:
                _nativeVerbs[it.integer()].value();
                break;
            default:
                // FIXME: Handle unknowns
                return;
        }
    }
}
