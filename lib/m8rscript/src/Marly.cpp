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
            case Token::Identifier:
                _code->push_back(_atomTable.atomizeString(scanner.getTokenValue().str));
                break;
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
            case Value::Type::Atom:
                printf("*** executing %s\n", it.string(this));
                break;
            default:
                // FIXME: Handle unknowns
                return;
        }
    }
}
