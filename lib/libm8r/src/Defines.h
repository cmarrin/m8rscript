/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

// Do this so we can present defines for malloc/free for c files
#ifndef __cplusplus

#else

#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <cassert>
#include <limits>
#include <cstring>

// Debugging
static inline void DBG_PRINT(const char* type, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("===== %s: ", type);
    vprintf(fmt, args);
    printf("\n");
}

//#define DEBUG_TIMERS
#ifdef DEBUG_TIMERS 
    #define DBG_TIMERS(fmt, ...) DBG_PRINT("TMR", fmt, ##__VA_ARGS__)
#else
    #define DBG_TIMERS(fmt, ...)
#endif

#include <chrono>

using namespace std::chrono_literals;

namespace m8r {

static constexpr uint8_t MajorVersion = 0;
static constexpr uint8_t MinorVersion = 2;

static inline bool isdigit(uint8_t c)		{ return c >= '0' && c <= '9'; }
static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
static inline bool isxdigit(uint8_t c)		{ return isHex(c) || isdigit(c); }
static inline bool isOctal(uint8_t c)       { return c >= '0' && c <= '7'; }
static inline bool isUpper(uint8_t c)		{ return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)		{ return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)		{ return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)		{ return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)		{ return isdigit(c) || isIdFirst(c); }
static inline bool isspace(uint8_t c)       { return c == ' ' || c == '\n' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }
static inline uint8_t tolower(uint8_t c)    { return isUpper(c) ? (c - 'A' + 'a') : c; }
static inline uint8_t toupper(uint8_t c)    { return isLower(c) ? (c - 'a' + 'A') : c; }

static inline bool isSpecial(uint8_t c)
{
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || ( c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

// KeyActions have a 1-4 character code which m8rscript can compare against. For instance
// 
//      function handleAction(action)
//      {
//          if (action == "down") ...
//      }
//
// Interrupt is control-c
//
// to make this work efficiently Action enumerants are uint32_t with the characters packed
// in. These are converted to StringLiteral Values and sent to the script.     

static constexpr uint32_t makeAction(const char* s)
{
    return
        (static_cast<uint32_t>(s[0]) << 24) |
        (static_cast<uint32_t>(s[1]) << 16) |
        (static_cast<uint32_t>(s[2]) << 8) |
        static_cast<uint32_t>(s[3]);
}

enum class KeyAction : uint32_t {
    None = 0,
    UpArrow = makeAction("up  "),
    DownArrow = makeAction("down"),
    RightArrow = makeAction("rt  "),
    LeftArrow = makeAction("lt  "),
    Delete = makeAction("del "),
    Backspace = makeAction("bs  "),
    Interrupt = makeAction("intr"),
    NewLine = makeAction("newl"),
};

//  Class: Id/RawId template
//
//  Generic Id class

template <typename RawType>
class Id
{
public:
    class Raw
    {
        friend class Id;

    public:
        Raw() : _raw(NoId) { }
        explicit Raw(RawType raw) : _raw(raw) { }
        RawType raw() const { return _raw; }

    private:
        RawType _raw;
    };
    
    using value_type = RawType;
    
    Id() { _value = Raw(NoId); }
    explicit Id(Raw raw) { _value._raw = raw._raw; }
    explicit Id(RawType raw) { _value._raw = raw; }
    Id(const Id& other) { _value._raw = other._value._raw; }
    Id(Id&& other) { _value._raw = other._value._raw; }

    value_type raw() const { return _value._raw; }

    const Id& operator=(const Id& other) { _value._raw = other._value._raw; return *this; }
    Id& operator=(Id& other) { _value._raw = other._value._raw; return *this; }
    const Id& operator=(const Raw& other) { _value._raw = other._raw; return *this; }
    Id& operator=(Raw& other) { _value._raw = other._raw; return *this; }
    explicit operator bool() const { return _value._raw != NoId; }

    int operator-(const Id& other) const { return static_cast<int>(_value._raw) - static_cast<int>(other._value._raw); }
    bool operator==(const Id& other) const { return _value._raw == other._value._raw; }

private:
    static constexpr RawType NoId = std::numeric_limits<RawType>::max();

    Raw _value;
};

enum class MemoryType : uint8_t {
    Unknown,
    String,
    Character,
    Object,
    ExecutionUnit,
    Native,
    Vector,
    UpValue,
    Network,
    Fixed,
    NumTypes
};

}

#endif
