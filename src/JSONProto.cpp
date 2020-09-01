/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "JSONProto.h"

#include "Defines.h"
#include "ExecutionUnit.h"
#include "StringStream.h"
#include "Scanner.h"
#include "SystemInterface.h"

using namespace m8rscript;
using namespace m8r;

static StaticObject::StaticFunctionProperty _props[] =
{
    { SA::parse, JSONProto::parseFunc },
    { SA::stringify, JSONProto::stringifyFunc },
};

JSONProto::JSONProto()
{
    setProperties(_props, sizeof(_props) / sizeof(StaticFunctionProperty));
}

Value JSONProto::value(ExecutionUnit* eu, Scanner& scanner)
{
    Value v;
    
    switch(scanner.getToken()) {
        case Token::Minus: 
            scanner.retireToken();
            switch(scanner.getToken()) {
                case Token::Float: v = Value(-scanner.getTokenValue().number); scanner.retireToken(); break;
                case Token::Integer: v = Value(-static_cast<int32_t>(scanner.getTokenValue().integer)); scanner.retireToken(); break;
                default: return Value();
            }
            break;
        case Token::Float: v = Value(scanner.getTokenValue().number); scanner.retireToken(); break;
        case Token::Integer: v = Value(static_cast<int32_t>(scanner.getTokenValue().integer)); scanner.retireToken(); break;
        case Token::String: v = Value(ExecutionUnit::createString(scanner.getTokenValue().str)); scanner.retireToken(); break;
        case Token::True: v = Value(static_cast<int32_t>(1)); scanner.retireToken(); break;
        case Token::False: v = Value(static_cast<int32_t>(0)); scanner.retireToken(); break;
        case Token::Null: v = Value::NullValue(); scanner.retireToken(); break;;
        case Token::LBracket: {
            scanner.retireToken();
            Mad<Object> mo = Object::create<MaterArray>();
            v = Value(mo);
            Value elementValue = value(eu, scanner);
            if (elementValue) {
                v.setElement(eu, Value(), elementValue, Value::SetType::AlwaysAdd);
                while (scanner.getToken() == Token::Comma) {
                    scanner.retireToken();
                    elementValue = value(eu, scanner);
                    if (!elementValue) {
                        eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(), 
                                                     "unable to add element to JSON Array").c_str());
                        return Value();
                    }
                    v.setElement(eu, Value(), elementValue, Value::SetType::AlwaysAdd);
                }
            }
            if (scanner.getToken() != Token::RBracket) {
                eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(), 
                                             "missing ']' in JSON Array").c_str());
                return Value();
            }
            scanner.retireToken();
            break;
        }
        case Token::LBrace: {
            scanner.retireToken();
            Mad<Object> obj = Object::create<MaterObject>();
            v = Value(obj);

            Value propertyKey;
            Value propertyValue;
            if (propertyAssignment(eu, scanner, propertyKey, propertyValue)) {
                v.setProperty(propertyKey.toIdValue(eu), propertyValue, Value::SetType::AlwaysAdd);
                while (scanner.getToken() == Token::Comma) {
                    scanner.retireToken();
                    if (!propertyAssignment(eu, scanner, propertyKey, propertyValue)) {
                        break;
                    }
                    if (!propertyValue) {
                        eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(), 
                                                     "invalid property value in JSON Object").c_str());
                        return Value();
                    }

                    v.setProperty(propertyKey.toIdValue(eu), propertyValue, Value::SetType::AlwaysAdd);
                }
            }
            if (scanner.getToken() != Token::RBrace) {
                eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(), 
                                             "missing '}' in JSON Object").c_str());
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

bool JSONProto::propertyAssignment(ExecutionUnit* eu, Scanner& scanner, Value& key, Value& v)
{
    if (scanner.getToken() != Token::String) {
        eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(), 
                                     "JSON property name must be a string").c_str());
        return false;
    }
    
    key = Value(ExecutionUnit::createString(scanner.getTokenValue().str));
    scanner.retireToken();
    if (scanner.getToken() != Token::Colon) {
        eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(), 
                                     "missing ':' in JSON Object").c_str());
        return false;
    }
    scanner.retireToken();
    v = value(eu, scanner);
    return static_cast<bool>(v);
}

Value JSONProto::parse(ExecutionUnit* eu, const String& json)
{
    StringStream stream(json);
    Scanner scanner(&stream);
    Value v = value(eu, scanner);
    if (scanner.getToken() != Token::EndOfFile) {
        eu->print(Error::formatError(Error::Code::RuntimeError, eu->lineno(), 
                                     "invalid token in JSON string").c_str());
        return Value();
    }
    return v;
}

m8r::String JSONProto::stringify(ExecutionUnit* eu, const Value v)
{
    return String();
}

CallReturnValue JSONProto::parseFunc(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    String s = eu->stack().top(1 - nparams).toStringValue(eu);
    Value v = parse(eu, s);
    eu->stack().push(v);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

CallReturnValue JSONProto::stringifyFunc(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams < 1) {
        return CallReturnValue(Error::Code::WrongNumberOfParams);
    }
    
    Value v = eu->stack().top(1 - nparams);
    String s = stringify(eu, v);
    eu->stack().push(Value(ExecutionUnit::createString(s)));
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}
