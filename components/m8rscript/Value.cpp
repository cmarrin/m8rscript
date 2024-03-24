/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Value.h"

#include "ExecutionUnit.h"
#include "Object.h"
#include <cmath>

using namespace m8rscript;
using namespace m8r;

m8r::String Value::toStringValue(ExecutionUnit* eu) const
{
    const char* s = toStringPointer(eu);
    if (s && s[0] != '\0') {
        return String(s);
    }
    
    switch(type()) {
        case Type::Object: {
            Mad<Object> obj = asObject();
            return obj.valid() ? obj->toString(eu) : String("null");
        }
        case Type::Float: return String(asFloatValue());
        case Type::Integer: return String(asIntValue());
        default: return String();
    }
}

const char* Value::toStringPointer(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::Undefined: return "undefined";
        default:
        case Type::Object:
        case Type::Float:
        case Type::Integer: return "";
        case Type::String: {
            Mad<String> s = asString();
            return s.valid() ? s->c_str() : "*BAD*";
        }
        case Type::StringLiteral: return eu->program()->stringFromStringLiteral(stringLiteralFromValue());
        case Type::Id: return eu->program()->stringFromAtom(atomFromValue());
        case Type::Null: return "null";
        case Type::NativeObject: return "Native()"; // FIXME: Add formatted toString and show the address
        case Type::NativeFunction: return "NativeFunction()"; // FIXME: Add formatted toString and show the address
        case Type::StaticObject: return "StaticObject()"; // FIXME: Add formatted toString and show the address
    }
}

float Value::_toFloatValue(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::Object: {
            Mad<Object> obj = asObject();
            float f = 0;
            if (obj.valid()) {
                String::toFloat(f, obj->toString(eu).c_str());
            }
            return f;
        }
        case Type::Float: return asFloatValue();
        case Type::Integer: return int32FromValue();
        case Type::String: {
            const Mad<String> s = asString();
            if (!s.valid()) {
                return 0;
            }
            float f;
            String::toFloat(f, s->c_str());
            return f;
        }
        case Type::StringLiteral: {
            const String& s = eu->program()->stringFromStringLiteral(stringLiteralFromValue());
            float f;
            String::toFloat(f, s.c_str());
            return f;
        }
        case Type::Id:
        case Type::NativeObject:
        case Type::NativeFunction:
        case Type::StaticObject:
        case Type::Null:
            return 0;
        case Type::Undefined:
        default:
            return NAN;
    }
}

Atom Value::_toIdValue(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::Object: {
            Mad<Object> obj = asObject();
            return obj.valid() ? eu->program()->atomizeString(obj->toString(eu).c_str()) : Atom();
        }
        case Type::Integer:
        case Type::Float: return eu->program()->atomizeString(toStringValue(eu).c_str());
        case Type::String: {
            const Mad<String> s = asString();
            return s.valid() ? eu->program()->atomizeString(s->c_str()) : Atom();
        }
        case Type::StringLiteral: {
            const String& s = eu->program()->stringFromStringLiteral(stringLiteralFromValue());
            return eu->program()->atomizeString(s.c_str());
        }
        case Type::Id:
        case Type::NativeObject:
        case Type::NativeFunction:
        case Type::StaticObject:
        case Type::Undefined:
        case Type::Null:
        default:
            return Atom();
    }
}

bool Value::isType(ExecutionUnit* eu, Atom atom)
{
    if (!isObject()) {
        return false;
    }
    Atom typeAtom = asObject()->typeName();
    return typeAtom == atom;
}

bool Value::isType(ExecutionUnit* eu, SA sa)
{
    return isType(eu, SAtom(sa));
}

const Value Value::property(const Atom& prop) const
{
    switch(type()) {
        case Type::Object: {
            Mad<Object> obj = asObject();
            return obj.valid() ? obj->property(prop) : Value();
        }
        case Type::Integer:
        case Type::Float: 
            // FIXME: Implement a Number object
            break;
        case Type::StringLiteral:
        case Type::String: {
            break;
        }
        case Type::StaticObject: {
            const StaticObject* obj = asStaticObject();
            if (obj) {
                return obj->property(prop);
            }
            break;
        }
        case Type::Id:
        case Type::NativeObject:
        case Type::NativeFunction:
        case Type::Undefined:
        case Type::Null:
        default:
            break;
    }
    return Value();
}

const Value Value::property(ExecutionUnit* eu, const Atom& prop) const
{
    if (type() == Type::String || type() == Type::StringLiteral) {
        if (prop == SAtom(SA::length)) {
            return Value(static_cast<int32_t>(toStringValue(eu).size()));
        }
    }
    return property(prop);
}

bool Value::setProperty(const Atom& prop, const Value& value, Value::SetType type)
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    Mad<Object> obj = asObject();
    return obj.valid() ? obj->setProperty(prop, value, type) : false;
}

const Value Value::element(ExecutionUnit* eu, const Value& elt) const
{
    if (isString()) {
        // This means String or StringLiteral
        int32_t index = elt.toIntValue(eu);
        const Mad<String> s = asString();
        if (s.valid()) {
            if (s->size() > index && index >= 0) {
                return Value(static_cast<int32_t>((*s)[index]));
            }
        } else {
            // Must be a string literal
            const char* s = eu->program()->stringFromStringLiteral(asStringLiteralValue());
            if (s) {
                if (static_cast<int32_t>(strlen(s)) > index && index >= 0) {
                    return Value(static_cast<int32_t>((s[index])));
                }
            }
        }
    } else {
        Mad<Object> obj = asObject();
        if (obj.valid()) {
            return obj->element(eu, elt);
        }
    }
    return Value();
}

bool Value::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, SetType type)
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    Mad<Object> obj = asObject();
    return obj.valid() ? obj->setElement(eu, elt, value, type) : false;
}

CallReturnValue Value::construct(ExecutionUnit* eu, uint32_t nparams)
{
    return Object::construct(*this, eu, nparams);
}

CallReturnValue Value::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    if (isNativeFunction()) {
        return asNativeFunction()(eu, thisValue, nparams);
    }

    Mad<Object> obj = asObject();
    return obj.valid() ? obj->call(eu, thisValue, nparams) : CallReturnValue(Error::Code::CannotCall);
}

CallReturnValue Value::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    switch(type()) {
        case Type::Object: {
            Mad<Object> obj = asObject();
            return obj.valid() ? obj->callProperty(eu, prop, nparams) : CallReturnValue(Error::Code::CannotCall);
        }
        case Type::Integer:
        case Type::Float: 
            // FIXME: Implement a Number object
            return CallReturnValue(Error::Code::CannotCall);
        case Type::StringLiteral:
        case Type::String: {
            String s = toStringValue(eu);

            if (prop == SAtom(SA::trim)) {
                s = s.trim();
                eu->stack().push(Value(ExecutionUnit::createString(s)));
                return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
            }
            if (prop == SAtom(SA::split)) {
                String separator = (nparams > 0) ? eu->stack().top(1 - nparams).toStringValue(eu) : String(" ");
                bool skipEmpty = (nparams > 1) ? eu->stack().top(2 - nparams).toBoolValue(eu) : false;
                Vector<String> array = s.split(separator, skipEmpty);
                Mad<MaterArray> arrayObject = Object::create<MaterArray>();
                for (int32_t i = 0; i < array.size(); ++i) {
                    arrayObject->setElement(eu, Value(i), Value(ExecutionUnit::createString(array[i])), Value::SetType::AlwaysAdd);
                }
                
                eu->stack().push(Value(Mad<Object>(static_cast<Mad<Object>>(arrayObject))));
                return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
            }
            return CallReturnValue(Error::Code::PropertyDoesNotExist);
        }
        case Type::StaticObject: {
            StaticObject* obj = asStaticObject();
            NativeFunction func = obj ? obj->property(prop).asNativeFunction() : nullptr;
            if (!func) {
                return CallReturnValue(Error::Code::CannotCall);
            }
            return func(eu, Value(), nparams);
        }
        case Type::Id:
        case Type::NativeObject:
        case Type::NativeFunction:
        case Type::Undefined:
        case Type::Null:
        default:
            return CallReturnValue(Error::Code::CannotCall);
    }
}

void Value::gcMark() const
{
    Mad<String> string = asString();
    if (string.valid()) {
        string->setMarked(true);
        return;
    }
    
    Mad<Object> obj = asObject();
    if (obj.valid()) {
        obj->gcMark();
    }
}
