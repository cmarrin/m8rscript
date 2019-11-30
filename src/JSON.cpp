/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "JSON.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "MStream.h"
#include "Scanner.h"
#include "SystemInterface.h"

using namespace m8r;

static StaticObject::StaticProperty RODATA2_ATTR _props[] =
{
    { SA::parse, Value(JSON::parseFunc) },
    { SA::stringify, Value(JSON::stringifyFunc) },
};

JSON::JSON()
{
    setProperties(_props, sizeof(_props) / sizeof(StaticProperty));
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
        case Token::String: v = Value(String::create(scanner.getTokenValue().str)); scanner.retireToken(); break;
        case Token::True: v = Value(static_cast<int32_t>(1)); scanner.retireToken(); break;
        case Token::False: v = Value(static_cast<int32_t>(0)); scanner.retireToken(); break;
        case Token::Null: v = Value::NullValue(); scanner.retireToken(); break;;
        case Token::LBracket: {
            scanner.retireToken();
            Mad<MaterObject> mo = Object::create<MaterObject>();
            mo->setArray(true);
            v = Value(mo);
            Value elementValue = value(eu, scanner);
            if (elementValue) {
                v.setElement(eu, Value(), elementValue, true);
                while (scanner.getToken() == Token::Comma) {
                    scanner.retireToken();
                    elementValue = value(eu, scanner);
                    if (!elementValue) {
                        Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("unable to add element to JSON Array"));
                        return Value();
                    }
                    v.setElement(eu, Value(), elementValue, true);
                }
            }
            if (scanner.getToken() != Token::RBracket) {
                Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("missing ']' in JSON Array"));
                return Value();
            }
            scanner.retireToken();
            break;
        }
        case Token::LBrace: {
            scanner.retireToken();
            v = Value(Object::create<MaterObject>());

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
                        Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("invalid property value in JSON Object"));
                        return Value();
                    }

                    v.setProperty(eu, propertyKey.toIdValue(eu), propertyValue, Value::SetPropertyType::AlwaysAdd);
                }
            }
            if (scanner.getToken() != Token::RBrace) {
                Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("missing '}' in JSON Object"));
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
        Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("JSON property name must be a string"));
        return false;
    }
    
    key = Value(String::create(scanner.getTokenValue().str));
    scanner.retireToken();
    if (scanner.getToken() != Token::Colon) {
        Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("missing ':' in JSON Object"));
        return false;
    }
    scanner.retireToken();
    v = value(eu, scanner);
    return static_cast<bool>(v);
}

Value JSON::parse(ExecutionUnit* eu, const String& json)
{
    StringStream stream(json);
    Scanner scanner(&stream);
    Value v = value(eu, scanner);
    if (scanner.getToken() != Token::EndOfFile) {
        Error::printError(eu, Error::Code::RuntimeError, eu->lineno(), ROMSTR("invalid token in JSON string"));
        return Value();
    }
    return v;
}

String JSON::stringify(ExecutionUnit* eu, const Value v)
{
    return String();
}

CallReturnValue JSON::parseFunc(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    Value v = parse(eu, s);
    eu->stack().push(v);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue JSON::stringifyFunc(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Value v = eu->stack().top(1 - nparams);
    String s = stringify(eu, v);
    eu->stack().push(Value(String::create(s)));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
