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

static int32_t intToString(Float::decompose_type x, char* str, int16_t dp)
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

static bool toString(char* buf, Float::decompose_type value, int16_t& exp)
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
    Float::decompose_type v = value;
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
    int16_t exp;
    Float::decompose_type mantissa;
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

m8r::String Value::toString(int32_t value)
{
    char buf[12];
    int16_t exp = 0;
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
    switch(type()) {
        case Type::None: return String("null");
        case Type::Object: {
            Value* v = nullptr;
            Object* obj = eu->program()->obj(*this);
            if (obj) {
                v = obj->value();
            }
            return v ? v->toStringValue(eu) : (obj ? obj->toString(eu) : String("unknown"));
        }
        case Type::Float: return toString(asFloatValue());
        case Type::Integer: return toString(asIntValue());
        case Type::String: return eu->program()->str(stringIdFromValue());
        case Type::StringLiteral: return eu->program()->stringFromStringLiteral(stringLiteralFromValue());
        case Type::Id: return m8r::String(eu->program()->stringFromAtom(atomFromValue()));
        default: assert(0); return m8r::String();
    }
}

Float Value::_toFloatValue(ExecutionUnit* eu) const
{
    switch(type()) {
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
        case Type::Integer: return Float(intFromValue(), 0);
        case Type::String: {
            const String& s = eu->program()->str(stringIdFromValue());
            return floatFromString(s.c_str());
        }
        case Type::StringLiteral: {
            const String& s = eu->program()->stringFromStringLiteral(stringLiteralFromValue());
            return floatFromString(s.c_str());
        }
        case Type::Id: break;
        default: assert(0); break;
    }
    return Float();
}

Atom Value::_toIdValue(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::None: break;
        case Type::Object: {
            Object* obj = eu->program()->obj(*this);
            Value* v = nullptr;
            if (obj) {
                v = obj->value();
            }
            if (v) {
                return v->toIdValue(eu);
            }
            break;
        }
        case Type::Integer:
        case Type::Float: return eu->program()->atomizeString(toStringValue(eu).c_str());
        case Type::String: {
            const String& s = eu->program()->str(stringIdFromValue());
            return eu->program()->atomizeString(s.c_str());
        }
        case Type::StringLiteral: {
            const String& s = eu->program()->stringFromStringLiteral(stringLiteralFromValue());
            return eu->program()->atomizeString(s.c_str());
        }
        default: assert(0); break;
    }
    return Atom();
}

void Value::_gcMark(ExecutionUnit* eu)
{
    eu->program()->gcMarkValue(eu, *this);
}
