/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#ifdef ARDUINO
#include "Arduino.h"
#endif

#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstring>

namespace m8r {

class String;

#ifndef ARDUINO
    #define RODATA_ATTR
    #define RODATA2_ATTR
    #define ROMSTR_ATTR
    #define ROMSTR(s) m8r::ROMString(s)
#else
    #define ROMSTR_ATTR PROGMEM
    #define RODATA_ATTR PROGMEM 
    #define RODATA2_ATTR PROGMEM
    #define ROMSTR(s) m8r::ROMString(PSTR(s))
#endif

    class ROMString
    {
    public:
        ROMString() { }
        explicit ROMString(const char* s) : _value(s) { }
        explicit ROMString(const uint8_t* s) : _value(reinterpret_cast<const char*>(s)) { }
        
        bool valid() const { return _value; }
        
        ROMString operator+(int32_t i) const { return ROMString(_value + i);  }
        ROMString operator-(int32_t i) const { return ROMString(_value - i);  }
        
#ifndef ARDUINO
        static uint8_t readByte(ROMString addr) { return *(addr._value); }
        static size_t strlen(ROMString s) { return ::strlen(s._value); }
        static void* memcpy(void* dst, ROMString src, size_t len) { return ::memcpy(dst, src._value, len); }
        static int strcmp(ROMString s1, const char* s2) { return ::strcmp(s1._value, s2); }
        static char* strcpy(char* dst, ROMString src) { return ::strcpy(dst, src._value); }
        static ROMString strstr(ROMString s1, const char* s2) { return ROMString(::strstr(s1._value, s2)); }
#else
        static uint8_t readByte(ROMString addr) { return pgm_read_byte(addr._value); }
        static size_t strlen(m8r::ROMString s) { return strlen_P(s._value); }
        static void* memcpy(void* dst, m8r::ROMString src, size_t len) { return memcpy_P(dst, src._value, len); }
        static int strcmp(m8r::ROMString s1, const char* s2) { return -strcmp_P(s2, s1._value); }
        static char* strcpy(char* dst, m8r::ROMString src) { return strcpy_P(dst, src._value); }
        static m8r::ROMString strstr(m8r::ROMString s1, const char* s2);
#endif

        String copy() const;
        
        static String vformat(ROMString format, va_list args);
        static String format(ROMString format, ...);

    private:
        const char* _value = nullptr;
    };
}
