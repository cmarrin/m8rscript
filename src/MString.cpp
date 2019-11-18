/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MString.h"

#include "MStream.h"
#include "Scanner.h"
#include "SystemInterface.h"
#include "slre.h"

using namespace m8r;

inline static void reverse(char *str, int len)
{
    for (int32_t i = 0, j = len - 1; i < j; i++, j--) {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
}

String& String::erase(uint16_t pos, uint16_t len)
{
    if (pos >= _size - 1) {
        return *this;
    }
    if (pos + len >= _size) {
        len = _size - pos - 1;
    }
    memmove(_data.get() + pos, _data.get() + pos + len, _size - pos - len);
    _size -= len;
    return *this;
}

String String::slice(int32_t start, int32_t end) const
{
    int32_t sz = static_cast<int32_t>(size());
    if (start < 0) {
        start = sz + start;
    }
    if (end < 0) {
        end = sz + end;
    }
    if (end > sz) {
        end = sz;
    }
    if (start >= end) {
        return String();
    }
    return String(_data.get() + start, end - start);
}

String String::trim() const
{
    if (_size < 2 || !_data.valid()) {
        return String();
    }
    uint16_t l = _size - 1;
    char* s = _data.get();
    while (isspace(s[l - 1])) {
        --l;
    }
    while (*s && isspace(*s)) {
        ++s;
        --l;
    }
    return String(s, static_cast<int32_t>(l));
}

Vector<String> String::split(const String& separator, bool skipEmpty) const
{
    Vector<String> array;
    char* p = _data.get();
    while (1) {
        char* n = strstr(p, separator.c_str());
        if (!n || n - p != 0 || !skipEmpty) {
            array.push_back(String(p, static_cast<int32_t>(n ? (n - p) : -1)));
        }
        if (!n) {
            break;
        }
        p = n ? (n + separator.size()) : nullptr;
    }
    return array;
}

String String::join(const Vector<String>& array, const String& separator)
{
    String s;
    bool first = true;
    for (auto it : array) {
        if (first) {
            first = false;
        } else {
            s += separator;
        }
        s += it;
    }
    return s;
}

String String::join(const Vector<char>& array)
{
    String s;
    s.ensureCapacity(array.size());
    for (auto it : array) {
        s += it;
    }
    return s;
}
void String::doEnsureCapacity(uint16_t size)
{
    uint16_t oldCapacity = _capacity;
    _capacity = _capacity ? _capacity * 2 : 1;
    if (_capacity < size) {
        _capacity = size;
    }
    Mad<char> newData = Mad<char>::create(_capacity);
    assert(newData.valid());
    if (_data.valid()) {
        if (newData.valid()) {
            memcpy(newData.get(), _data.get(), _size);
        } else {
            _capacity = 0;
            _size = 1;
        }
        _data.destroy(oldCapacity);
    }
    _data = newData;
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

String String::toString(Float value)
{
    //          sign    digits  dp      'e'     dp      exp     '\0'
    char buf[   1 +     16 +    1 +     1 +     1 +     3 +     1];
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

String String::toString(int32_t value)
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

bool String::toFloat(Float& f, const char* s, bool allowWhitespace)
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

bool String::toInt(int32_t& i, const char* s, bool allowWhitespace)
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

bool String::toUInt(uint32_t& u, const char* s, bool allowWhitespace)
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

String String::fformat(const char* fmt, std::function<Value(FormatType)> func)
{
    if (!fmt || fmt[0] == '\0') {
        return String();
    }
    
    String resultString;
    
    static const char* formatRegexROM = ROMSTR("(%)([\\d]*)(.?)([\\d]*)([c|s|d|i|x|X|u|f|e|E|g|G|p])");
        
    uint16_t formatRegexSize = ROMstrlen(formatRegexROM) + 1;
    Mad<char> formatRegex = Mad<char>::create(formatRegexSize);
    ROMmemcpy(formatRegex.get(), formatRegexROM, formatRegexSize);
    
    int size = static_cast<int>(strlen(fmt));
    const char* start = fmt;
    const char* s = start;
    while (true) {
        struct slre_cap caps[5];
        memset(caps, 0, sizeof(caps));
        int next = slre_match(formatRegex.get(), s, size - static_cast<int>(s - start), caps, 5, 0);
        if (next == SLRE_NO_MATCH) {
            // Print the remainder of the string
            resultString += s;
            formatRegex.destroy(formatRegexSize);
            return resultString;
        }
        if (next < 0) {
            formatRegex.destroy(formatRegexSize);
            return String();
        }
        
        // Output anything from s to the '%'
        assert(caps[0].len == 1);
        if (s != caps[0].ptr) {
            resultString += String(s, static_cast<int32_t>(caps[0].ptr - s));
        }
        
        // FIXME: handle the leading number(s) in the format
        assert(caps[4].len == 1);
        
        uint32_t width = 0;
        bool zeroFill = false;
        if (caps[1].len) {
            String::toUInt(width, caps[1].ptr);
            if (caps[1].ptr[0] == '0') {
                zeroFill = true;
            }
        }
        
        char formatChar = *(caps[4].ptr);
        
        switch (formatChar) {
            case 'c': {
                uint8_t uc = static_cast<char>(func(FormatType::Int).asIntValue());
                char escapeChar = '\0';
                switch(uc) {
                    case 0x07: escapeChar = 'a'; break;
                    case 0x08: escapeChar = 'b'; break;
                    case 0x09: escapeChar = 't'; break;
                    case 0x0a: escapeChar = 'n'; break;
                    case 0x0b: escapeChar = 'v'; break;
                    case 0x0c: escapeChar = 'f'; break;
                    case 0x0d: escapeChar = 'r'; break;
                    case 0x1b: escapeChar = 'e'; break;
                }
                if (escapeChar) {
                    resultString += '\\';
                    resultString += escapeChar;
                } else if (uc < ' ') {
                    char buf[4] = "";
                    ::snprintf(buf, 3, "%02x", uc);
                    resultString += "\\x";
                    resultString += buf;
                } else {
                    resultString += static_cast<char>(uc);
                }
                break;
            }
            case 's':
                resultString += func(FormatType::String).asString().get()->c_str();
                break;
            case 'd':
            case 'i':
            case 'x':
            case 'X':
            case 'u': {
                String format = String("%") + (zeroFill ? "0" : "") + (width ? String::toString(width).c_str() : "");
                char numberBuf[20] = "";
                
                switch(formatChar) {
                    case 'd':
                    case 'i':
                        format += "d";
                        ::snprintf(numberBuf, 20, format.c_str(), func(FormatType::Int).asIntValue());
                        break;
                    case 'x':
                    case 'X':
                        format += (formatChar == 'x') ? "x" : "X";
                        ::snprintf(numberBuf, 20, format.c_str(), static_cast<uint32_t>(func(FormatType::Int).asIntValue()));
                        break;
                    case 'u':
                        format += "u";
                        ::snprintf(numberBuf, 20, format.c_str(), static_cast<uint32_t>(func(FormatType::Int).asIntValue()));
                        break;
                }
                
                resultString += numberBuf;
                break;
            }
            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                resultString += toString(func(FormatType::Float).asFloatValue());
                break;
            case 'p': {
                resultString += func(FormatType::Ptr).asString().get()->c_str();
                break;
            }
            default:
                formatRegex.destroy(formatRegexSize);
                return String();
        }
        
        s += next;
    }
}

String String::vformat(const char* fmt, va_list args)
{
    String s = fformat(fmt, [&args](String::FormatType type) {
        switch(type) {
            case String::FormatType::Int:
                return Value(static_cast<int32_t>(va_arg(args, int)));
            case String::FormatType::String:
                return Value(Mad<String>::create(va_arg(args, const char*)));
            case String::FormatType::Float:
                // TODO: Implement
                va_arg(args, double);
                return Value(Float());
            case String::FormatType::Ptr: {
                // TODO: Implement
                va_arg(args, void*);
                return Value();
            }
        }
    });
    return s;
}
