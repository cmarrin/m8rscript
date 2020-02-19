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
#include <ctime>
#include <chrono>

using namespace m8r;

Time Time::now()
{
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    return Time(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
}

String Time::toString() const
{
    Elements elts;
    elements(elts);
    
    // For now just show h:m:s.ms
    return String::format("%d:%02d:%02d.%d", elts.hour, elts.minute, elts.second, elts.us / 1000);
}

void Time::elements(Elements& elts) const
{
    elts.us = static_cast<uint32_t>(_value % 1000000);
    time_t currentSeconds = static_cast<time_t>(_value / 1000000);

    struct tm* timeStruct = localtime(&currentSeconds);
    elts.year = timeStruct->tm_year;
    elts.month = timeStruct->tm_mon;
    elts.day = timeStruct->tm_mday;
    elts.hour = timeStruct->tm_hour;
    elts.minute = timeStruct->tm_min;
    elts.second = timeStruct->tm_sec;
    elts.dayOfWeek = static_cast<DayOfWeek>(timeStruct->tm_wday);
}

String Duration::toString(Duration::Units units, uint8_t decimalDigits) const
{
    Float f = toFloat();
    switch(units) {
        default:
        case Duration::Units::ms: return String(f * Float(1000), decimalDigits) + "ms";
        case Duration::Units::us: return String(f * Float(1000000), decimalDigits) + "us";
        case Duration::Units::sec: return String(f, decimalDigits) + "sec";
    }
}
