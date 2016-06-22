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
    
    Float(uint32_t i, uint32_t f, uint8_t dp, int32_t e)
    {
#ifdef FIXED_POINT_FLOAT
        int32_t num = i*1000;
        while (dp < 3) {
            ++dp;
            f *= 10;
        }
        while (dp > 3) {
            --dp;
            f /= 10;
        }
        while ( f > 1000) {
            f /= 10;
        }
        num += f;
        while (e > 0) {
            --e;
            num *= 10;
        }
        while (e < 0) {
            ++e;
            num /= 10;
        }
        Float f;
        f._value._f = num;
        return f;
#else
        float num = (float) f;
        while (dp-- > 0)
            num /= 10;
        num += (float) i;
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
#else
    Float operator*(const Float& other) const { Float r; r._value._f = _value._f * other._value._f; return r; }
    Float operator/(const Float& other) const { Float r; r._value._f = _value._f / other._value._f; return r; }
    Float floor() const { Float r; r._value._f = static_cast<float>(static_cast<int32_t>(_value._f)); return r; }
    operator uint32_t() const { return static_cast<uint32_t>(_value._f); }
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
