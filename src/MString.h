/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>
#include <functional>
#include <vector>
#include "Containers.h"
#include "Defines.h"
#include "Float.h"
#include "Mallocator.h"

namespace m8r {

class Value;

//
//  Class: String
//
//  String class that works on both Mac and ESP
//

class String {
public:
    static MemoryType memoryType() { return MemoryType::String; }

    String() : _size(1), _capacity(0) { }
    String(const char* s, int32_t len = -1) : _size(1), _capacity(0)
    {
        if (!s) {
            return;
        }
        if (len == -1) {
            len = static_cast<int32_t>(strlen(s));
        }
        ensureCapacity(len + 1);
        if (len) {
            memcpy(_data.get(), s, len);
        }
        _size = len + 1;
        _data.get()[_size - 1] = '\0';
    }
    
    String(ROMString s)
    {
        *this = s;
    }
    
    String(const String& other)
    {
        *this = other;
    };
    
    String(String&& other)
    {
        _data.destroy(_capacity);
        _data = other._data;
        other._data = Mad<char>();
        _size = other._size;
        other._size = 0;
        _capacity = other._capacity;
        other._capacity = 0;
    }
    
    String(char c)
    {
        ensureCapacity(2);
        char* s = _data.get();
        s[0] = c;
        s[1] = '\0';
        _size = 2;
    }
    
    ~String() { _data.destroy(_capacity); _destroyed = true; };

    String& operator=(ROMString other)
    {
        uint16_t romSize = static_cast<uint16_t>(ROMstrlen(other));
        
        if (_data.valid()) {
            _size = 0;
            if (_capacity < romSize) {
                _data.destroy(_capacity);
                _data = Mad<char>();
                _capacity = 0;
            }
        }
        
        ensureCapacity(romSize + 1);
        _size = romSize + 1;
        if (romSize > 0) {
            ROMmemcpy(_data.get(), other, romSize);
        }
        _data.get()[romSize] = '\0';
        return *this;
    }

    String& operator=(const String& other)
    {
        _data.destroy(_capacity);
        _size = other._size;
        _capacity = other._capacity;
        if (!other._data.valid()) {
            _data = Mad<char>();
            return *this;
        }
        
        _data = Mad<char>::create(_capacity);
        assert(_data.valid());
        if (_data.valid()) {
            memcpy(_data.get(), other._data.get(), _size);
        } else {
            _capacity = 0;
            _size = 1;
        }
        return *this;
    }
    
    String& operator=(String&& other)
    {
        if (this == &other) {
            return *this;
        }

        _data.destroy(_capacity);

        _data = other._data;
        _size = other._size;
        _capacity = other._capacity;

        other._data = Mad<char>();
        other._size = 0;
        other._capacity = 0;

        return *this;
    }
    
    String& operator=(char c)
    {
        ensureCapacity(2);
        char* s = _data.get();
        s[0] = c;
        s[1] = '\0';
        _size = 2;
        return *this;
    }

    operator bool () { return !empty(); }
    
    const char& operator[](uint16_t i) const { assert(i >= 0 && i < _size - 1); return _data.get()[i]; };
    char& operator[](uint16_t i) { assert(i >= 0 && i < _size - 1); return _data.get()[i]; };
    const char& at(uint16_t i) const { assert(i >= 0 && i < _size - 1); return _data.get()[i]; };
    char& at(uint16_t i) { assert(i >= 0 && i < _size - 1); return _data.get()[i]; };
    
    char& back() { return at(size() - 1); }
    const char& back() const { return at(size() - 1); }

    char& front() { return at(0); }
    const char& front() const { return at(0); }

    uint16_t size() const { return _size ? (_size - 1) : 0; }
    bool empty() const { return _size <= 1; }
    void clear() { _size = 1; if (_data.valid()) _data.get()[0] = '\0'; }
    String& operator+=(uint8_t c)
    {
        ensureCapacity(_size + 1);
        char* s = _data.get();
        s[_size - 1] = c;
        s[_size++] = '\0';
        return *this;
    }
    
    String& operator+=(char c)
    {
        ensureCapacity(_size + 1);
        char* s = _data.get();
        s[_size - 1] = c;
        s[_size] = '\0';
        _size += 1;
        return *this;
    }
    
    String& operator+=(const char* s)
    {
        uint16_t len = strlen(s);
        ensureCapacity(_size + len);
        memcpy(_data.get() + _size - 1, s, len + 1);
        _size += len;
        return *this;
    }
    
    String& operator+=(const String& s) { return *this += s.c_str(); }
    
    friend String operator +(const String& s1 , const String& s2) { String s = s1; s += s2; return s; }
    friend String operator +(const String& s1 , const char* s2) { String s = s1; s += s2; return s; }
    friend String operator +(const String& s1 , char c) { String s = s1; s += c; return s; }
    friend String operator +(const char* s1 , const String& s2) { String s = s1; s += s2; return s; }
    
    bool operator<(const String& other) const { return compare(*this, other) < 0; }
    bool operator<=(const String& other) const { return compare(*this, other) <= 0; }
    bool operator>(const String& other) const { return compare(*this, other) > 0; }
    bool operator>=(const String& other) const { return compare(*this, other) >= 0; }
    bool operator==(const String& other) const { return compare(*this, other) == 0; }
    bool operator!=(const String& other) const { return compare(*this, other) != 0; }
    
    static int compare(const String& a, const String& b)
    {
        return strcmp(a.c_str(), b.c_str());
    }

    const char* c_str() const { return _data.valid() ? _data.get() : ""; }
    String& erase(uint16_t pos, uint16_t len);

    String& erase(uint16_t pos = 0)
    {
        return erase(pos, _size - pos);
    }
    
    String slice(int32_t start, int32_t end) const;
    
    String slice(int32_t start) const
    {
        return slice(start, static_cast<int32_t>(size()));
    }
    
    String trim() const;
    
    // If skipEmpty is true, substrings of zero length are not added to the array
    Vector<String> split(const String& separator, bool skipEmpty = false) const;
    
    static String join(const Vector<String>& array, const String& separator);
    
    static String join(const Vector<char>& array);
    
    bool isMarked() const { return _marked; }
    void setMarked(bool b) { _marked = b; }
    
    void reserve(uint16_t size) { ensureCapacity(size); }
    
    static String toString(Float value);
    static String toString(int32_t value);
    static bool toFloat(Float&, const char*, bool allowWhitespace = true);
    static bool toInt(int32_t&, const char*, bool allowWhitespace = true);
    static bool toUInt(uint32_t&, const char*, bool allowWhitespace = true);
    
    enum class FormatType { Int, String, Float, Ptr };
    
    static String fformat(const char* fmt, std::function<Value(FormatType)>);
    static String vformat(const char*, va_list);
    static String vformat(ROMString, va_list);
    static String format(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        return vformat(fmt, args);
    }

private:
    void doEnsureCapacity(uint16_t size);
    
    void ensureCapacity(uint16_t size)
    {
        if (_capacity >= size) {
            return;
        }
        doEnsureCapacity(size);
    }

    uint16_t _size = 0;
    uint16_t _capacity = 0;
    Mad<char> _data;
    bool _marked = true;
    bool _destroyed = false;
};

}
