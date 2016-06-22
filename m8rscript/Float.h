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

#pragma once

#ifndef FIXED_POINT_FLOAT
#include <cmath>
#endif

#include <cstdint>

namespace m8r {

class RawFloat
{
    friend class Float;

public:
    uint32_t raw() const
    {
#ifdef FIXED_POINT_FLOAT
        return _f;
#else
        return *(reinterpret_cast<uint32_t*>(const_cast<float*>(&_f)));
#endif
    }
    
private:
#ifdef FIXED_POINT_FLOAT
    int32_t _f;
#else
    float _f;
#endif
};

class Float
{
public:
    // Max number of digits we will ever print out (integer+fraction)
    static constexpr uint8_t MaxDigits = 10;

    Float() { _value._f = 0; }
    Float(const RawFloat& value) { _value._f = value._f; }
    Float(const Float& value) { _value._f = value._value._f; }
    Float(Float& value) { _value._f = value._value._f; }
    Float(uint32_t v)
    {
#ifdef FIXED_POINT_FLOAT
        _value._f = static_cast<int32_t>(v);
#else
        _value._f = *(reinterpret_cast<float*>(&v));
#endif
    }
    
    Float(int32_t i, int32_t e)
    {
#ifdef FIXED_POINT_FLOAT
        int32_t num = i*1000;
        while (e > 3) {
            --e;
            num *= 10;
        }
        while (e < 3) {
            ++e;
            num /= 10;
        }
        Float f;
        f._value._f = num;
        return f;
#else
        float num = (float) i;
        while (e > 0) {
            --e;
            num *= 10;
        }
        while (e < 0) {
            ++e;
            num /= 10;
        }
        _value._f = num;
#endif
    }
    
    const Float& operator=(const Float& other) { _value._f = other._value._f; return *this; }
    Float& operator=(Float& other) { _value._f = other._value._f; return *this; }
    
    Float operator+(const Float& other) const { Float r; r._value._f = _value._f + other._value._f; return r; }
    Float operator-(const Float& other) const { Float r; r._value._f = _value._f - other._value._f; return r; }

#ifdef FIXED_POINT_FLOAT
    Float operator*(const Float& other) const { Float r; r._value._f = _value._f * other._value._f / 1000; return r; }
    Float operator/(const Float& other) const { Float r; r._value._f = _value._f * 1000 / other._value._f; return r; }
    Float floor() const { Float r; r._value._f = _value._f / 1000 * 1000; return r; }
    operator uint32_t() { return _value._f; }

    int32_t mantissa() const { return _value._f; }
    int32_t exponent() const { return -3; }
#else
    Float operator*(const Float& other) const { Float r; r._value._f = _value._f * other._value._f; return r; }
    Float operator/(const Float& other) const { Float r; r._value._f = _value._f / other._value._f; return r; }
    Float floor() const { Float r; r._value._f = static_cast<float>(static_cast<int32_t>(_value._f)); return r; }
    operator uint32_t() const { return static_cast<uint32_t>(_value._f); }

    void decompose(int32_t& mantissa, int32_t& exponent) const
    {
        if (_value._f == 0) {
            mantissa = 0;
            exponent = 0;
            return;
        }
        int32_t sign = (_value._f < 0) ? -1 : 1;
        float value = _value._f * sign;
        int32_t exp = 0;
        while (value >= 1) {
            value /= 10;
            exp++;
        }
        while (value < 0.1) {
            value *= 10;
            exp--;
        }
        mantissa = static_cast<int32_t>(value * 1000000000);
        exponent = exp - 9;
    }
    int32_t exponent() const
    {
        int exp;
        std::frexp(_value._f, &exp);
        return static_cast<int32_t>(exp - 9);
    }

#endif

    Float operator%(const Float& other) { return *this - other * (*this / other).floor(); }
    
    bool operator==(const Float& other) const { return _value._f == other._value._f; }
    bool operator!=(const Float& other) const { return _value._f != other._value._f; }
    bool operator<(const Float& other) const { return _value._f < other._value._f; }
    bool operator<=(const Float& other) const { return _value._f <= other._value._f; }
    bool operator>(const Float& other) const { return _value._f > other._value._f; }
    bool operator>=(const Float& other) const { return _value._f >= other._value._f; }

    Float operator-() const { Float r; r._value._f = -_value._f; return r; }
    operator int32_t() const { return static_cast<int32_t>(static_cast<uint32_t>(*this)); }
    operator RawFloat() const { return _value; }

private:
    RawFloat _value;
};

}
