/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
	  
    - Redistributions in binary form must reproduce the above copyright 
	  notice, this list of conditions and the following disclaimer in the 
	  documentation and/or other materials provided with the distribution.
	  
    - Neither the name of the <ORGANIZATION> nor the names of its 
	  contributors may be used to endorse or promote products derived from 
	  this software without specific prior written permission.
	  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "JSON.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "MStream.h"
#include "Scanner.h"

using namespace m8r;

JSON::JSON(Program* program)
    : ObjectFactory(program, ROMSTR("JSON"))
    , _parse(parse)
    , _stringify(stringify)
{
    addProperty(program, ATOM(parse), &_parse);
    addProperty(program, ATOM(stringify), &_stringify);
}

Value JSON::value(ExecutionUnit* eu, Scanner& scanner)
{
    Value v;
    
    switch(scanner.getToken()) {
        case Token::Minus: 
            scanner.retireToken();
            switch(scanner.getToken()) {
                case Token::Float: v = Value(-Float(scanner.getTokenValue().number)); scanner.retireToken(); break;
                case Token::Integer: v = Value(-static_cast<int32_t>(scanner.getTokenValue().integer)); scanner.retireToken(); break;
                default: return Value();
            }
            break;
        case Token::Float: v = Value(Float(scanner.getTokenValue().number)); scanner.retireToken(); break;
        case Token::Integer: v = Value(static_cast<int32_t>(scanner.getTokenValue().integer)); scanner.retireToken(); break;
        case Token::String: v = Value(Object::createString(scanner.getTokenValue().str)); scanner.retireToken(); break;
        case Token::True: v = Value(static_cast<int32_t>(1)); scanner.retireToken(); break;
        case Token::False: v = Value(static_cast<int32_t>(0)); scanner.retireToken(); break;
        case Token::Null: v = Value(Value::Type::Null); scanner.retireToken(); break;;
        case Token::LBracket: {
            scanner.retireToken();
            v = Value(new Array());
            Value elementValue = value(eu, scanner);
            if (elementValue) {
                v.setElement(eu, Value(), elementValue, true);
                while (scanner.getToken() == Token::Comma) {
                    scanner.retireToken();
                    elementValue = value(eu, scanner);
                    if (!elementValue) {
                        Error::printError(eu->program()->system(), Error::Code::RuntimeError, eu->lineno(), ROMSTR("unable to add element to JSON Array"));
                        return Value();
                    }
                    v.setElement(eu, Value(), elementValue, true);
                }
            }
            if (scanner.getToken() != Token::RBracket) {
                Error::printError(eu->program()->system(), Error::Code::RuntimeError, eu->lineno(), ROMSTR("missing ']' in JSON Array"));
                return Value();
            }
            scanner.retireToken();
            break;
        }
        case Token::LBrace: {
            scanner.retireToken();
            v = Value(new MaterObject());

            Value propertyKey;
            Value propertyValue;
            if (propertyAssignment(eu, scanner, propertyKey, propertyValue)) {
                v.setProperty(eu, propertyKey.toIdValue(eu), propertyValue, Value::SetPropertyType::AlwaysAdd);
                while (scanner.getToken() == Token::Comma) {
                    scanner.retireToken();
                    if (!propertyAssignment(eu, scanner, propertyKey, propertyValue)) {
                        break;
                    }
                    if (!propertyValue) {
                        Error::printError(eu->program()->system(), Error::Code::RuntimeError, eu->lineno(), ROMSTR("invalid property value in JSON Object"));
                        return Value();
                    }

                    v.setProperty(eu, propertyKey.toIdValue(eu), propertyValue, Value::SetPropertyType::AlwaysAdd);
                }
            }
            if (scanner.getToken() != Token::RBrace) {
                Error::printError(eu->program()->system(), Error::Code::RuntimeError, eu->lineno(), ROMSTR("missing '}' in JSON Object"));
                return Value();
            }
            scanner.retireToken();
            break;
        }
        default:
            return Value();
    }
    return v;
}

bool JSON::propertyAssignment(ExecutionUnit* eu, Scanner& scanner, Value& key, Value& v)
{
    if (scanner.getToken() != Token::String) {
        Error::printError(eu->program()->system(), Error::Code::RuntimeError, eu->lineno(), ROMSTR("JSON property name must be a string"));
        return false;
    }
    
    key = Value(Object::createString(scanner.getTokenValue().str));
    scanner.retireToken();
    if (scanner.getToken() != Token::Colon) {
        Error::printError(eu->program()->system(), Error::Code::RuntimeError, eu->lineno(), ROMSTR("missing ':' in JSON Object"));
        return false;
    }
    scanner.retireToken();
    v = value(eu, scanner);
    return v;
}

Value JSON::parse(ExecutionUnit* eu, const String& json)
{
    StringStream stream(json);
    Scanner scanner(&stream);
    Value v = value(eu, scanner);
    if (scanner.getToken() != Token::EndOfFile) {
        Error::printError(eu->program()->system(), Error::Code::RuntimeError, eu->lineno(), ROMSTR("invalid token in JSON string"));
        return Value();
    }
    return v;
}

String JSON::stringify(ExecutionUnit* eu, const Value v)
{
    return String();
}

CallReturnValue JSON::parse(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    Value v = parse(eu, s);
    eu->stack().push(v);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue JSON::stringify(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    Value v = eu->stack().top(1 - nparams);
    String s = stringify(eu, v);
    eu->stack().push(Value(Object::createString(s)));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
