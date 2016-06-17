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

#include "Value.h"

#include "Object.h"
#include <cstdio>

using namespace m8r;

m8r::String Value::toString(float value)
{
    char buf[40];
    sprintf(buf, "%g", value);
    return m8r::String(buf);
}

m8r::String Value::toString(uint32_t value)
{
    char buf[20];
    sprintf(buf, "%u", value);
    return m8r::String(buf);
}

m8r::String Value::toString(int32_t value)
{
    char buf[20];
    sprintf(buf, "%d", value);
    return m8r::String(buf);
}

m8r::String Value::toStringValue() const
{
    switch(_type) {
        case Type::ValuePtr:
            return bakeValue().toStringValue();
        case Type::None: return String();
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = asObjectValue();
            if (obj) {
                v = obj->value();
            }
            return v ? v->toStringValue() : String();
        }
        case Type::Float: return toString(asFloatValue());
        case Type::Integer: return toString(asIntValue());
        case Type::String: return m8r::String(asStringValue());
        case Type::Id: return m8r::String();
        case Type::Ref: return objFromValue()->property(_id).toStringValue();
        case Type::Return: assert(0); return m8r::String();
    }
}

bool Value::toBoolValue() const
{
    switch(_type) {
        case Type::ValuePtr:
            return bakeValue().toBoolValue();
        case Type::None: return false;
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = asObjectValue();
            if (obj) {
                v = obj->value();
            }
            return v ? v->toBoolValue() : false;
        }
        case Type::Float: return asFloatValue() != 0;
        case Type::Integer: return asIntValue() != 0;
        case Type::String: {
            const char* s = asStringValue();
            return s ? (s[0] != '\0') : false;
        }
        case Type::Id: return false;
        case Type::Ref:
            return objFromValue()->property(_id).toBoolValue();
        case Type::Return: assert(0); return false;
    }
}

float Value::toFloatValue() const
{
    switch(_type) {
        case Type::ValuePtr:
            return bakeValue().toFloatValue();
        case Type::None: return 0;
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = asObjectValue();
            if (obj) {
                v = obj->value();
            }
            return v ? v->toFloatValue() : 0;
        }
        case Type::Float: return asFloatValue();
        case Type::Integer: return static_cast<float>(asIntValue());
        case Type::String: {
            const char* s = asStringValue();
            return s ? std::atof(s) : 0;
        }
        case Type::Id: return 0;
        case Type::Ref:
            return objFromValue()->property(_id).toFloatValue();
        case Type::Return: assert(0); return 0;
    }
}

bool Value::setValue(const Value& v)
{
    Value bakedValue = v.bakeValue();
    switch(_type) {
        case Type::Object:
            return objFromValue()->setValue(bakedValue);
        case Type::Ref:
            return objFromValue()->setProperty(_id, bakedValue);
        case Type::ValuePtr:
            *valuePtrFromValue() = bakedValue;
            return true;
        default:
            return false;
    }
}

Object* Value::toObjectValue() const
{
    if (_type == Type::Object) {
        return objFromValue();
    }
    if (_type == Type::ValuePtr) {
        return valuePtrFromValue()->toObjectValue();
    }
    return nullptr;
}

Value Value::appendPropertyRef(const Value& value) const
{
    return (_type == Type::Ref) ? objFromValue()->appendPropertyRef(_id, value.asIdValue()) : Value();
}

uint32_t Value::call(Stack<Value>& stack, uint32_t nparams)
{
    if (_type == Type::Ref) {
        return objFromValue()->callProperty(_id, stack, nparams);
    }
    return -1;
}

    

