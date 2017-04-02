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
#include "MStream.h"
#include "Object.h"
#include "Program.h"
#include "Scanner.h"

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

bool Value::toFloat(Float& f, const char* s, bool allowWhitespace)
{
    StringStream stream(s);
    Scanner scanner(&stream);
    bool neg = false;
    Scanner::TokenType type;
  	Token token = scanner.getToken(type, allowWhitespace);
    if (token == Token::Minus) {
        neg = true;
        token = scanner.getToken(type, allowWhitespace);
    }
    if (token == Token::Float || token == Token::Integer) {
        f = (token == Token::Float) ? Float(type.number) : Float(type.integer, 0);
        if (neg) {
            f = -f;
        }
        return true;
    }
    return false;
}

bool Value::toInt(int32_t& i, const char* s, bool allowWhitespace)
{
    StringStream stream(s);
    Scanner scanner(&stream);
    bool neg = false;
    Scanner::TokenType type;
  	Token token = scanner.getToken(type, allowWhitespace);
    if (token == Token::Minus) {
        neg = true;
        token = scanner.getToken(type, allowWhitespace);
    }
    if (token == Token::Integer && type.integer <= std::numeric_limits<int32_t>::max()) {
        i = type.integer;
        if (neg) {
            i = -i;
        }
        return true;
    }
    return false;
}

bool Value::toUInt(uint32_t& u, const char* s, bool allowWhitespace)
{
    StringStream stream(s);
    Scanner scanner(&stream);
    Scanner::TokenType type;
  	Token token = scanner.getToken(type, allowWhitespace);
    if (token == Token::Integer) {
        u = type.integer;
        return true;
    }
    return false;
}

m8r::String Value::toStringValue(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::None: return String("null");
        case Type::Object: {
            Object* obj = Global::obj(*this);
            return obj ? obj->toString(eu) : String("null");
        }
        case Type::Float: return toString(asFloatValue());
        case Type::Integer: return toString(asIntValue());
        case Type::String: return Global::str(stringIdFromValue());
        case Type::StringLiteral: return eu->program()->stringFromStringLiteral(stringLiteralFromValue());
        case Type::Id: return String(eu->program()->stringFromAtom(atomFromValue()));
        case Type::Null: return String("null");
        case Type::NativeObject: return String("Native()"); // FIXME: Add formatted toString and show the address
        case Type::Function: return String("Function()"); // FIXME: Show some sort of function identifier, name or something
    }
}

Float Value::_toFloatValue(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::Object: {
            Object* obj = Global::obj(*this);
            Float f;
            if (obj) {
                toFloat(f, obj->toString(eu).c_str());
            }
            return f;
        }
        case Type::Float: return asFloatValue();
        case Type::Integer: return Float(int32FromValue(), 0);
        case Type::String: {
            const String& s = Global::str(stringIdFromValue());
            Float f;
            toFloat(f, s.c_str());
            return f;
        }
        case Type::StringLiteral: {
            const String& s = eu->program()->stringFromStringLiteral(stringLiteralFromValue());
            Float f;
            toFloat(f, s.c_str());
            return f;
        }
        case Type::Id:
        case Type::NativeObject:
        case Type::Function:
        case Type::None:
        case Type::Null:
            return Float();
    }
}

Atom Value::_toIdValue(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::Object: {
            Object* obj = Global::obj(*this);
            return obj ? eu->program()->atomizeString(obj->toString(eu).c_str()) : Atom();
        }
        case Type::Integer:
        case Type::Float: return eu->program()->atomizeString(toStringValue(eu).c_str());
        case Type::String: {
            const String& s = Global::str(stringIdFromValue());
            return eu->program()->atomizeString(s.c_str());
        }
        case Type::StringLiteral: {
            const String& s = eu->program()->stringFromStringLiteral(stringLiteralFromValue());
            return eu->program()->atomizeString(s.c_str());
        }
        case Type::Id:
        case Type::NativeObject:
        case Type::Function:
        case Type::None:
        case Type::Null:
            return Atom();
    }
}

Object* Value::toObject(ExecutionUnit* eu) const
{
    switch(type()) {
        case Type::Object:
            return Global::obj(asObjectIdValue());
        case Type::Function: {
            Callable* callable = asFunction();
            switch(callable->type()) {
                case Callable::Type::None: return nullptr;
                case Callable::Type::Function: return static_cast<Function*>(callable);
                case Callable::Type::Closure: return static_cast<Closure*>(callable);
            };
        }
        case Type::Integer:
        case Type::Float: 
            // FIXME: Implement a Number object
            break;
        case Type::StringLiteral:
        case Type::String:
            // FIXME: Implement a String object
            break;
        case Type::Id:
        case Type::NativeObject:
        case Type::None:
        case Type::Null:
            break;
    }
    return nullptr;
}

const Value Value::property(ExecutionUnit* eu, const Atom& atom) const
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    Object* obj = toObject(eu);
    return obj ? obj->property(eu, atom) : Value();
}

bool Value::setProperty(ExecutionUnit* eu, const Atom& prop, const Value& value, Value::SetPropertyType type)
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    Object* obj = toObject(eu);
    return obj ? obj->setProperty(eu, prop, value, type) : false;
}

const Value Value::element(ExecutionUnit* eu, const Value& elt) const
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    Object* obj = toObject(eu);
    return obj ? obj->element(eu, elt) : Value();
}

bool Value::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    Object* obj = toObject(eu);
    return obj ? obj->setElement(eu, elt, value, append) : false;
}

CallReturnValue Value::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    // FIXME: Handle Integer, Float, String and StringLiteral
    Object* obj = toObject(eu);
    return obj ? obj->call(eu, thisValue, nparams, ctor) : CallReturnValue(CallReturnValue::Type::Error);
}

CallReturnValue Value::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    Object* obj = toObject(eu);
    return obj ? obj->callProperty(eu, prop, nparams) : CallReturnValue(CallReturnValue::Type::Error);
}

void Value::_gcMark(ExecutionUnit* eu)
{
    Global::gcMark(eu, *this);
}
