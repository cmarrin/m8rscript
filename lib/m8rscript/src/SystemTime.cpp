/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "SystemTime.h"

#include "SystemInterface.h"
#include <chrono>

using namespace m8r;

uint64_t Time::_baseTime = 0;

Time Time::now()
{
    if (_baseTime == 0) {
        _baseTime = 1; // to avoid loop when we recurse
        _baseTime = now().us();
        assert(_baseTime > 0);
    }
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    uint64_t t = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    t -= _baseTime;
    return Time(t);
}

m8r::String Time::toString() const
{
    Elements elts;
    elements(elts);
    
    // For now just show h:m:s.ms
    return String::format("%d:%02d:%02d.%03d", elts.hour, elts.minute, elts.second, elts.us / 1000);
}

void Time::elements(Elements& elts) const
{
    elts.us = static_cast<uint32_t>(_value % 1000000);
    time_t currentSeconds = static_cast<time_t>(_value / 1000000);

    struct tm* timeStruct = localtime(&currentSeconds);
    elts.year = timeStruct->tm_year + 1900;
    elts.month = timeStruct->tm_mon;
    elts.day = timeStruct->tm_mday;
    elts.hour = timeStruct->tm_hour;
    elts.minute = timeStruct->tm_min;
    elts.second = timeStruct->tm_sec;
    elts.dayOfWeek = static_cast<DayOfWeek>(timeStruct->tm_wday);
}

m8r::String Duration::toString(Duration::Units units, uint8_t decimalDigits) const
{
    double f = toFloat();
    switch(units) {
        default:
        case Duration::Units::ms: return String(f * 1000, decimalDigits) + "ms";
        case Duration::Units::us: return String(f * 1000000, decimalDigits) + "us";
        case Duration::Units::sec: return String(f, decimalDigits) + "sec";
    }
}

String Time::Elements::dayString() const
{
    static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    return String(days[int(dayOfWeek)]);
}

String Time::Elements::monthString() const
{
    static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    return String(months[month]);
}
