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

#include "ExecutionUnit.h"
#include "Object.h"
#include "Program.h"

using namespace m8r;

inline static void reverse(char *str, int len)
{
    for (int32_t i = 0, j = len - 1; i < j; i++, j--) {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
}

static int32_t intToString(int32_t x, char* str, int32_t dp)
{
    int32_t i = 0;
    bool haveDP = false;
    
    while (x) {
        str[i++] = (x % 10) + '0';
        x /= 10;
        if (--dp == 0) {
            str[i++] = '.';
            haveDP = true;
        }
    }
    
    if (dp > 0) {
        while (dp--) {
            str[i++] = '0';
        }
        str[i++] = '.';
        haveDP = true;
    }
    assert(i > 0);
    if (str[i-1] == '.') {
        str[i++] = '0';
    }
    
    reverse(str, i);
    str[i] = '\0';

    if (haveDP) {
        i--;
        while (str[i] == '0') {
            str[i--] = '\0';
        }
        if (str[i] == '.') {
            str[i--] = '\0';
        }
        i++;
    }

    return i;
}

static bool toString(char* buf, uint32_t value, int32_t& exp)
{
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        exp = 0;
        return true;
    }
    
    if (!exp) {
        intToString(value, buf, 0);
        return true;
    }

    // See how many digits we have
    uint32_t v = value;
    int digits = 0;
    for ( ; v > 0; ++digits, v /= 10) ;
    v = value;
    int32_t dp;
    if (exp + digits > Float::MaxDigits || -exp > Float::MaxDigits) {
        // Scientific notation
        dp = digits - 1;
        exp += dp;
    } else {
        dp = -exp;
        exp = 0;
    }
    
    int32_t i = intToString(value, buf, dp);
    if (exp) {
        buf[i++] = 'e';
        if (exp < 0) {
            buf[i++] = '-';
            exp = -exp;
        }
        intToString(exp, buf + i, 0);
    }
    
    return true;
}

m8r::String Value::toString(Float value)
{
    char buf[Float::MaxDigits + 8];
    int32_t exp;
    int32_t mantissa;
    value.decompose(mantissa, exp);
    if (mantissa < 0) {
        buf[0] = '-';
        mantissa = - mantissa;
        ::toString(buf + 1, mantissa, exp);
    } else {
        ::toString(buf, mantissa, exp);
    }
    return m8r::String(buf);
}

m8r::String Value::toString(uint32_t value)
{
    char buf[12];
    int32_t exp = 0;
    ::toString(buf, value, exp);
    return m8r::String(buf);
}

m8r::String Value::toString(int32_t value)
{
    char buf[12];
    int32_t exp = 0;
    if (value < 0) {
        buf[0] = '-';
        value = -value;
        ::toString(buf + 1, value, exp);
    } else {
        ::toString(buf, value, exp);
    }
    return m8r::String(buf);
}

Float Value::floatFromString(const char* s)
{
    // FIXME: implement
    return Float();
}

m8r::String Value::toStringValue(ExecutionUnit* eu) const
{
    switch(_type) {
        case Type::None: return String("null");
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = eu->program()->obj(*this);
            if (obj) {
                v = obj->value();
            }
            return v ? v->toStringValue(eu) : String();
        }
        case Type::Float: return toString(asFloatValue());
        case Type::Integer: return toString(asIntValue());
        case Type::String: return m8r::String(asStringValue());
        case Type::Id: return m8r::String(eu->program()->stringFromAtom(asIdValue()));
        case Type::PropertyRef: {
            Object* obj = eu->program()->obj(*this);
            return obj ? obj->property(_id).toStringValue(eu) : m8r::String();
        }
        case Type::ElementRef: {
            Object* obj = eu->program()->obj(*this);
            return obj ? obj->element(_id).toStringValue(eu) : m8r::String();
        }
        default: assert(0); return m8r::String();
    }
}

bool Value::toBoolValue(ExecutionUnit* eu) const
{
    switch(_type) {
        case Type::None: return false;
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = eu->program()->obj(*this);
            if (obj) {
                v = obj->value();
            }
            return v ? v->toBoolValue(eu) : false;
        }
        case Type::Float: return asFloatValue() != Float();
        case Type::Integer: return asIntValue() != 0;
        case Type::String: {
            const char* s = asStringValue();
            return s ? (s[0] != '\0') : false;
        }
        case Type::Id: return false;
        case Type::PropertyRef: {
            Object* obj = eu->program()->obj(*this);
            return obj ? obj->property(_id).toBoolValue(eu) : false;
        }
        case Type::ElementRef: {
            Object* obj = eu->program()->obj(*this);
            return obj ? obj->element(_id).toBoolValue(eu) : false;
        }
        default: assert(0); return false;
    }
}

Float Value::toFloatValue(ExecutionUnit* eu) const
{
    if (canBeBaked()) {
        return bake(eu).toFloatValue(eu);
    }
    switch(_type) {
        case Type::None: break;
        case Type::Object: {
            Object* obj = eu->program()->obj(*this);
            Value* v = nullptr;
            if (obj) {
                v = obj->value();
            }
            if (v) {
                return v->toFloatValue(eu);
            }
            break;
        }
        case Type::Float: return asFloatValue();
        case Type::Integer: return Float(asIntValue(), 0);
        case Type::String: {
            const char* s = asStringValue();
            if (s) {
                return floatFromString(s);
            }
            break;
        }
        case Type::Id: break;
        default: assert(0); break;
    }
    return Float();
}

bool Value::setValue(ExecutionUnit* eu, const Value& v)
{
    Object* obj;
    
    switch(_type) {
        case Type::Object:
            obj = eu->program()->obj(*this);
            return obj ? (obj->setValue(eu, v.canBeBaked() ? v.bake(eu) : v)) : false;
        case Type::PropertyRef:
            obj = eu->program()->obj(*this);
            return obj ? (obj->setProperty(eu, _id, v.canBeBaked() ? v.bake(eu) : v)) : false;
        case Type::ElementRef:
            obj = eu->program()->obj(*this);
            return obj ? (obj->setElement(eu, _id, v.canBeBaked() ? v.bake(eu) : v)) : false;
        default:
            return false;
    }
}

bool Value::deref(ExecutionUnit* eu, const Value& derefValue)
{
    assert(_type != Type::Id);
    assert(*this);
    
    Object* obj = nullptr;
    ObjectId objectId = asObjectIdValue();
    if (objectId) {
        obj = eu->program()->obj(objectId);
    }
    
    if (!obj) {
        Error::printError(eu->system(), Error::Code::RuntimeError, ROMSTR("'%s' object does not exist"), toStringValue(eu).c_str());
        return false;
    }
    
    // Fast path for indexing arrays
    if (derefValue.isNumber()) {
        int32_t index = derefValue.toIntValue(eu);
        Value ref = obj->elementRef(index);
        if (ref) {
            *this = ref;
            return true;
        }
        
        // Handle like a property
        String prop = Value::toString(index);
        *this = obj->propertyRef(obj->propertyIndex(eu->program()->atomizeString(Value::toString(index).c_str())));
        return true;
    }
        
    if (derefValue.isAtom()) {
        Atom atom = derefValue.asIdValue();
        
        if (type() == Value::Type::PropertyRef) {
            *this = obj->appendPropertyRef(_id, atom);
            return true;
        }
        
        int32_t index = obj->propertyIndex(atom);
        if (index < 0) {
            Error::printError(eu->system(), Error::Code::RuntimeError, ROMSTR("'%s' property does not exist"), eu->program()->stringFromAtom(atom).c_str());
            return false;
        }
        
        *this = obj->propertyRef(index);
        return true;
    }
    
    Value bakedValue = derefValue.bake(eu);
    switch(bakedValue.type()) {
        default: return Value();
        case Value::Type::String: {
            int32_t index = obj->propertyIndex(eu->program()->atomizeString(derefValue.asStringValue()));
            if (index < 0) {
                Error::printError(eu->system(), Error::Code::RuntimeError, ROMSTR("'%s' property does not exist"), derefValue.asStringValue());
                return false;
            }
            *this = obj->propertyRef(index);
            return true;
        }
        case Type::Float:
        case Type::Integer:
            *this = obj->elementRef(bakedValue.toIntValue(eu));
            return true;
    }
}

CallReturnValue Value::call(ExecutionUnit* eu, uint32_t nparams)
{
    if (_type == Type::PropertyRef) {
        Object* obj = eu->program()->obj(*this);
        return obj ? obj->callProperty(_id, eu, nparams) : CallReturnValue(CallReturnValue::Type::Error);
    }
    if (_type == Type::Object) {
        Object* obj = eu->program()->obj(*this);
        return obj ? obj->call(eu, nparams) : CallReturnValue(CallReturnValue::Type::Error);
    }
    return CallReturnValue(CallReturnValue::Type::Error);
}

Value Value::bake(ExecutionUnit* eu) const
{
    if (_type == Type::PropertyRef) {
        Object* obj = eu->program()->obj(*this);
        return obj ? obj->property(_id) : Value();
    }
    if (_type == Type::ElementRef) {
        Object* obj = eu->program()->obj(*this);
        return obj ? obj->element(_id) : Value();
    }
    return *this;
}
