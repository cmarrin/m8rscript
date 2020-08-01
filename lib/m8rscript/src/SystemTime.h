/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Defines.h"
#include <cstdint>
#include <limits>
#include <unistd.h>

namespace m8r {

class String;

// A Duration is a 32 bit number representing time. The upper 30 bits
// is an integer value. The lower 2 bits represents the units of the value:
//
//      0 - microseconds (range is 8.9 minutes)
//      1 - milliseconds (range is 6.2 days)
//      2 - seconds      (range is 139 years)
//      3 - reserved

class Duration
{
public:
    static constexpr uint32_t Shift = 2;
    static constexpr uint32_t UnitsMask = (1 << Shift) - 1;
    static constexpr uint32_t MaxValue = (1 << (sizeof(int32_t) * 8 - Shift)) - 1;
    
    enum class Units { us = 0, ms = 1, sec = 2, none = 3 };

    constexpr Duration() { }
    constexpr Duration(const Duration& other) { _value = other._value; }
    constexpr Duration(Duration&& other) { _value = other._value; }
    
    constexpr Duration(int64_t value)
    {
        int32_t v = (value > MaxValue) ? MaxValue : static_cast<int32_t>(value);
        _value = (v << Shift);
    }
    
    Duration(double value) { *this = Duration(static_cast<int64_t>(value * 1000000)); }
    
    constexpr Duration(std::chrono::microseconds value) { *this = value; }
    constexpr Duration(std::chrono::milliseconds value) { *this = value; }
    constexpr Duration(std::chrono::seconds value) { *this = value; }
    
    operator bool() { return us() != 0; }
    
    Duration operator - ()
    {
        Units u = units();
        _value = -(_value & ~UnitsMask);
        setUnits(u);
        return *this;
    }
    
    constexpr Duration& operator=(const Duration& other) { _value = other._value; return *this; }
    constexpr Duration& operator=(Duration&& other) { _value = other._value; return *this; }
    
    constexpr Duration& operator=(std::chrono::microseconds value)
    {
        *this = Duration(value.count());
        return *this;
    }
    
    constexpr Duration& operator=(std::chrono::milliseconds value)
    {
        *this = Duration(Duration(std::chrono::duration_cast<std::chrono::microseconds>(value)));
        return *this;
    }
    
    constexpr Duration& operator=(std::chrono::seconds value)
    {
        *this = Duration(Duration(std::chrono::duration_cast<std::chrono::microseconds>(value)));
        return *this;
    }

    Duration operator+(const Duration& other) const { return Duration(us() + other.us()); }
    Duration operator-(const Duration& other) const { return Duration(us() - other.us()); }

    Duration operator+=(const Duration& other) { *this = *this + other; return *this; }
    Duration operator-=(const Duration& other) { *this = *this - other; return *this; }

    bool operator==(const Duration& other) const { return us() == other.us(); }
    bool operator!=(const Duration& other) const { return us() != other.us(); }
    bool operator<(const Duration& other)  const { return us() < other.us(); }
    bool operator<=(const Duration& other) const { return us() <= other.us(); }
    bool operator>(const Duration& other)  const { return us() > other.us(); }
    bool operator>=(const Duration& other) const { return us() >= other.us(); }

    int64_t us() const
    {
        int64_t v = _value >> Shift;
        switch (units()) {
            case Units::ms: v *= 1000; break;
            case Units::sec: v *= 1000000; break;
            default: break;
        }
        return v;
    }

    int32_t ms() const
    {
        int32_t v = _value >> Shift;
        switch (units()) {
            case Units::us: v /= 1000; break;
            case Units::sec: v *= 1000; break;
            default: break;
        }
        return v;
    }
    
    double toFloat() const
    {
        double f(_value >> Shift);
        switch (units()) {
            case Units::us: f /= 1000000; break;
            case Units::ms: f /= 1000; break;
            default: break;
        }
        return f;
    }
    
    String toString(Duration::Units = Duration::Units::ms, uint8_t decimalDigits = 2) const;
    
    void sleep() const
    {
        if (ms() < 0) {
            return;
        }
        usleep(static_cast<uint32_t>(us()));
    }

private:
    constexpr Units units() const { return static_cast<Units>(_value & UnitsMask); }
    void setUnits(Units u) { _value = (_value & ~UnitsMask) | static_cast<int32_t>(u); }
    
    int32_t _value = 0;
};

// A Time value is an absolute time starting when the system started.

class Time
{
public:
    enum class DayOfWeek : uint8_t { Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday };
    
    struct Elements
    {
        uint32_t us = 0;
        int16_t year = 0;
        int8_t month = 0;
        int8_t day = 0;
        int8_t hour = 0;
        int8_t minute = 0;
        int8_t second = 0;
        DayOfWeek dayOfWeek = DayOfWeek::Sunday;
        
        String dayString() const;
        String monthString() const;
    };
    
    Time() { }
    Time(const Time& other) : _value(other.us()) { }

    static Time now();
    static Time longestTime() { return Time(std::numeric_limits<uint64_t>::max()); }
    
    friend Time operator+(const Time& t, const Duration& d) { return Time(t._value + d.us()); }
    friend Time operator+(const Duration& d, const Time& t) { return Time(t._value + d.us()); }
    friend Time operator-(const Time& t, const Duration& d) { return Time(t._value - d.us()); }
    friend Duration operator-(const Time& t1, const Time& t2) { return Duration(int64_t(t1._value - t2._value)); }

    Time operator+=(const Duration& other) { *this = *this + other; return *this; }
    Time operator-=(const Duration& other) { *this = *this - other; return *this; }

    bool operator==(const Time& other) const { return _value == other._value; }
    bool operator!=(const Time& other) const { return _value != other._value; }
    bool operator<(const Time& other)  const { return _value < other._value; }
    bool operator<=(const Time& other) const { return _value <= other._value; }
    bool operator>(const Time& other)  const { return _value > other._value; }
    bool operator>=(const Time& other) const { return _value >= other._value; }
    
    uint64_t us() const { return _value; }
    
    String toString() const;
    
    void elements(Elements&) const;

private:
    Time(uint64_t t) : _value(t) { }
    
    uint64_t _value = 0;
    
    static uint64_t _baseTime;
};

}
