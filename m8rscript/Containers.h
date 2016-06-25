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

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>

namespace m8r {

class RawStringLiteral
{
    friend class StringLiteral;
    
public:
    uint32_t raw() const { return _index; }
    static RawStringLiteral make(uint32_t raw) { RawStringLiteral r; r._index = raw; return r; }
    
private:
    uint32_t _index;
};

class StringLiteral {
    friend class Program;
    
public:
    StringLiteral() { _id._index = NoString; }
    StringLiteral(RawStringLiteral raw) { _id._index = raw._index; }
    StringLiteral(const StringLiteral& other) { _id._index = other._id._index; }
    StringLiteral(StringLiteral& other) { _id._index = other._id._index; }

    const StringLiteral& operator=(const StringLiteral& other) { _id._index = other._id._index; return *this; }
    StringLiteral& operator=(StringLiteral& other) { _id._index = other._id._index; return *this; }
    operator RawStringLiteral() const { return _id; }
    
    int operator-(const StringLiteral& other) const { return static_cast<int>(_id._index) - static_cast<int>(other._id._index); }
    
private:
    static constexpr uint32_t NoString = std::numeric_limits<uint32_t>::max();
    RawStringLiteral _id;
};
    
//////////////////////////////////////////////////////////////////////////////
//
//  Class: String
//
//  Wrapper String class that works on both Mac and ESP
//
//////////////////////////////////////////////////////////////////////////////

class String {
public:
	String() : _size(1), _capacity(0), _data(nullptr) { }
	String(const char* s, int32_t len = -1) : _size(1), _capacity(0), _data(nullptr)
    {
        if (len == -1) {
            len = static_cast<int32_t>(strlen(s));
        }
        ensureCapacity(len + 1);
        memcpy(_data, s, len);
        _size = len + 1;
        _data[_size - 1] = '\0';
    }
    
    String(const String& other) : _data(nullptr)
    {
        *this = other;
    };
    
    ~String() { free(_data); };

    String& operator=(const String& other)
    {
        if (_data) {
            free(_data);
        }
        _size = other._size;
        _capacity = other._capacity;
        _data = other._data ? static_cast<char *>(malloc(_capacity)) : nullptr;
        if (other._data) {
            memcpy(_data, other._data, _size);
        }
        return *this;
    }
    
    const char& operator[](size_t i) const { assert(i >= 0 && i < _size - 1); return _data[i]; };
    char& operator[](size_t i) { assert(i >= 0 && i < _size - 1); return _data[i]; };
	size_t length() const { return _size - 1; }
	String& operator+=(uint8_t c)
    {
        ensureCapacity(_size + 1);
        _data[_size - 1] = c;
        _data[_size++] = '\0';
        return *this;
    }
    
	String& operator+=(const char* s)
    {
        size_t len = strlen(s);
        ensureCapacity(_size + len);
        memcpy(_data + _size - 1, s, len + 1);
        _size += len;
        return *this;
    }
    
    String& operator+=(const String& s) { return *this += s.c_str(); }
    
    bool operator<(const String& other) const { return strcmp(c_str(), other.c_str()) < 0; }

    const char* c_str() const { return _data ? _data : ""; }
    void erase()
    {
        _size = 1;
        if (_data) {
            _data[0] = '\0';
        }
    }
	
private:
    void ensureCapacity(size_t size)
    {
        if (_capacity >= size) {
            return;
        }
        _capacity = _capacity ? _capacity * 2 : 1;
        if (_capacity < size) {
            _capacity = size;
        }
        char *newData = static_cast<char *>(malloc(_capacity));
        if (_data) {
            memcpy(newData, _data, _size);
            free(_data);
        }
        _data = newData;
    };

    size_t _size;
    size_t _capacity;
    char *_data;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Vector
//
//  Wrapper Vector class that works on both Mac and ESP
//
//////////////////////////////////////////////////////////////////////////////

template<typename type>
class Vector {
public:
    Vector() : _size(0), _capacity(0), _data(nullptr) { }
    Vector(Vector const &other) : _data(nullptr)
    {
        *this = other;
    };
    
    ~Vector() { free(_data); };
    
    Vector &operator=(Vector const &other)
    {
        if (_data) {
            free(_data);
        }
        _size = other._size;
        _capacity = other._capacity;
        _data = static_cast<type *>(malloc(_capacity*sizeof(type)));
        memcpy(_data, other._data, _size * sizeof(type));
        return *this;
    };

    void push_back(type const &x)
    {
        ensureCapacity(_size + 1);
        _data[_size++] = x;
    };
    
    void pop_back() { _size--; }
    
    bool empty() const { return !_size; }
    void clear() { resize(0); }
    size_t size() const { return _size; };
    void reserve(size_t size) { ensureCapacity(size); }
    void resize(size_t size)
    {
        ensureCapacity(size);
        if (size > _size) {
            memset(_data + _size, 0, (size - _size) * sizeof(type));
        }
        _size = size;
    }
    
    type& operator[](size_t i) { return at(i); };
    const type& operator[](size_t i) const { return at(i); };
    
    type& at(size_t i) { assert(i >= 0 && i < _size); return _data[i]; };
    const type& at(size_t i) const { assert(i >= 0 && i < _size); return _data[i]; };
    
    type& back() { return _data[_size - 1]; }
    const type& back() const { return _data[_size - 1]; }
    
    type* begin() { return _data; }
    const type* begin() const { return _data; }
    type* end() { return _data + _size; }
    const type* end() const { return _data + _size; }
    
private:
    void ensureCapacity(size_t size) { if (_capacity >= size) return; _ensureCapacity(size); }
    void _ensureCapacity(size_t size)
    {
        if (_capacity >= size) {
            return;
        }
        _capacity = _capacity ? _capacity * 2 : 1;
        if (_capacity < size) {
            _capacity = size;
        }
        type *newData = static_cast<type *>(malloc(_capacity * sizeof(type)));
        memcpy(newData, _data, _size * sizeof(type));
        free(_data);
        _data = newData;
    };

    size_t _size;
    size_t _capacity;
    type *_data;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Map
//
//  Wrapper Map class that works on both Mac and ESP
//  Simle ordered array. Done this way to minimize space
//
//////////////////////////////////////////////////////////////////////////////

template<class T>
struct CompareKeys
{
    int operator()(const T& lhs, const T& rhs) const { return lhs - rhs; }
};

template<typename Key, typename Value, typename Compare=CompareKeys<Key>>
class Map {
public:
    struct Pair
    {
        bool operator==(const Pair& other) const { return key == other.key; }
        Key key;
        Value value;
    };

    bool find(const Key& key, Value& value) const
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        if (result < 0) {
            return false;
        }
        value = _list[result].value;
        return true;
    }
    Value* emplace(const Key& key, const Value& value)
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        if (result < 0) {
            _list.resize(_list.size() + 1);
            int sizeToMove = static_cast<int>(_list.size()) + result;
            if (sizeToMove) {
                memmove(&_list[-result], &_list[-result - 1], sizeToMove * sizeof(Pair));
            }
            result = -result - 1;
            _list[result] = { key, value };
        }
        return &(_list[result].value);
    }
    
    Pair* begin() { return _list.size() ? &(_list[0]) : nullptr; }
    const Pair* begin() const { return _list.size() ? &(_list[0]) : nullptr; }
    Pair* end() { return _list.size() ? (&(_list[0]) + _list.size()) : nullptr; }
    const Pair* end() const { return _list.size() ? (&(_list[0]) + _list.size()) : nullptr; }
    
    bool empty() const { return _list.empty(); }
    size_t size() const { return _list.size(); }

private:
    int search(int first, int last, const Key& key) const
    {
        if (first <= last) {
            int mid = (first + last) / 2;
            int result = _compare(key, _list[mid].key);
            return (result == 0) ? mid : ((result < 0) ? search(first, mid - 1, key) : search(mid + 1, last, key));
        }
        return -(first + 1);    // failed to find key
    }
    
    Vector<Pair> _list;
    Compare _compare;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Stack
//
//  Wrapper around Vector to give stack semantics
//
//////////////////////////////////////////////////////////////////////////////

template<typename type>
class Stack : Vector<type> {
    typedef Vector<type> super;
    
public:
    Stack() { }
    Stack(size_t reserveCount) { super::reserve(reserveCount); }
    
    void clear() { super::clear(); }
    size_t size() const { return super::size(); }
    void push(const type& value) { super::push_back(value); }
    type& top(int32_t relative = 0)
    {
        assert(static_cast<int32_t>(super::size()) + relative > 0);
        return super::at(static_cast<int32_t>(super::size()) + relative - 1);
    }
    
    size_t setLocalFrame(size_t localSize)
    {
        size_t oldFrame = _frame;
        _frame = size();
        super::resize(size() + localSize);
        return oldFrame;
    }
    void restoreFrame(size_t frame)
    {
        assert(frame <= size() && frame <= _frame);
        super::resize(_frame);
        _frame = frame;
    }
    
    type& inFrame(int32_t index) { return super::at(_frame + index); }
    const type& inFrame(int32_t index) const { return super::at(_frame + index); }
    void pop(size_t n = 1) { super::resize(size() - n); }
    void setTop(const type& value) { super::back() = value; }

private:
    size_t _frame = 0;
};

}
