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
        while (--dp) {
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

m8r::String Value::toStringValue() const
{
    switch(_type) {
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
        case Type::PropertyRef: return objFromValue()->property(_id).toStringValue();
        case Type::ElementRef: return objFromValue()->element(_id).toStringValue();
        case Type::Return: assert(0); return m8r::String();
    }
}

bool Value::toBoolValue() const
{
    switch(_type) {
        case Type::None: return false;
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = asObjectValue();
            if (obj) {
                v = obj->value();
            }
            return v ? v->toBoolValue() : false;
        }
        case Type::Float: return asFloatValue() != Float();
        case Type::Integer: return asIntValue() != 0;
        case Type::String: {
            const char* s = asStringValue();
            return s ? (s[0] != '\0') : false;
        }
        case Type::Id: return false;
        case Type::PropertyRef: return objFromValue()->property(_id).toBoolValue();
        case Type::ElementRef: return objFromValue()->element(_id).toBoolValue();
        case Type::Return: assert(0); return false;
    }
}

Float Value::toFloatValue() const
{
    if (canBeBaked()) {
        return bakeValue().toFloatValue();
    }
    switch(_type) {
        case Type::None: break;
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = asObjectValue();
            if (obj) {
                v = obj->value();
            }
            if (v) {
                return v->toFloatValue();
            }
            break;
        }
        case Type::Float: return asFloatValue();
        case Type::Integer: return Float(asIntValue());
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

bool Value::setValue(const Value& v)
{
    Value bakedValue = v.bakeValue();
    switch(_type) {
        case Type::Object:
            return objFromValue()->setValue(bakedValue);
        case Type::PropertyRef:
            return objFromValue()->setProperty(_id, bakedValue);
        case Type::ElementRef:
            objFromValue()->setElement(_id, bakedValue);
            return true;
        default:
            return false;
    }
}

Object* Value::toObjectValue() const
{
    if (canBeBaked()) {
        return bakeValue().toObjectValue();
    }
    if (_type == Type::Object) {
        return objFromValue();
    }
    return nullptr;
}

Value Value::appendPropertyRef(const Value& value) const
{
    return (_type == Type::PropertyRef) ? objFromValue()->appendPropertyRef(_id, value.asIdValue()) : Value();
}

uint32_t Value::call(Stack<Value>& stack, uint32_t nparams)
{
    if (_type == Type::PropertyRef) {
        return objFromValue()->callProperty(_id, stack, nparams);
    }
    if (_type == Type::Object) {
        return objFromValue()->call(stack, nparams);
    }
    return -1;
}

Value Value::bakeValue() const
{
    if (_type == Type::PropertyRef) {
        return objFromValue()->property(_id);
    }
    if (_type == Type::ElementRef) {
        return objFromValue()->element(_id);
    }
    return *this;
}
