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

#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <limits>

namespace m8r {

class StringId {
    friend class Program;
    
public:
    static StringId emptyStringId() { StringId s; s._id = NoString; return s; }
    uint32_t rawStringId() const { return _id; }
    static StringId stringIdFromRawStringId(uint32_t id)
    {
        StringId s;
        s._id = id;
        return s;
    }
    int compare(const StringId& other) const { return static_cast<int>(_id) - static_cast<int>(other._id); }
    
private:
    static constexpr uint32_t NoString = std::numeric_limits<uint32_t>::max();
    uint32_t _id;
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
	String(const char* s, size_t len = -1) : _size(1), _capacity(0), _data(nullptr)
    {
        set(s, len);
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
    
    void set(const char* s, size_t len = -1)
    {
        if (len == -1) {
            len = strlen(s);
        }
        ensureCapacity(len + 1);
        memcpy(_data, s, len);
        _size = len + 1;
        _data[_size - 1] = '\0';
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

    const char* c_str() const { return _data; }
    void clear()
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
    Vector() : _size(0), _capacity(0), _data(nullptr) {}; // Default constructor
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
    
    size_t size() const { return _size; };
    void resize(size_t size)
    {
        ensureCapacity(size);
        
        _size = size;
    }
    
    const type& operator[](size_t i) const { assert(i >= 0 && i < _size); return _data[i]; };
    type& operator[](size_t i) { assert(i >= 0 && i < _size); return _data[i]; };
    
    type& back() { return _data[_size - 1]; }
    const type& back() const { return _data[_size - 1]; }
    
    type* begin() { return _data; }
    const type* begin() const { return _data; }
    type* end() { return _data + _size; }
    const type* end() const { return _data + _size; }
    
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

template<typename Key, typename Value>
class Map {
public:
    typedef struct { Key key; Value value; } Pair;

    Value* find(const Key& key)
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        return (result >= 0) ? &(_list[result].value) : nullptr;
    }
    Value* emplace(const Key& key, const Value& value)
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        if (result < 0) {
            _list.resize(_list.size() + 1);
            int sizeToMove = static_cast<int>(_list.size()) + result;
            if (sizeToMove) {
                memcpy(&_list[-result], &_list[-result - 1], sizeToMove);
            }
            _list[-result - 1] = { key, value };
        }
        return &(_list[-result - 1].value);
    }
    
    Pair* begin() { return _list.size() ? &(_list[0]) : nullptr; }
    const Pair* begin() const { return _list.size() ? &(_list[0]) : nullptr; }
    Pair* end() { return _list.size() ? (&(_list[0]) + _list.size()) : nullptr; }
    const Pair* end() const { return _list.size() ? (&(_list[0]) + _list.size()) : nullptr; }

private:
    int search(int first, int last, const Key& key)
    {
        if (first <= last) {
            int mid = (first + last) / 2;
            int result = key.compare(_list[mid].key);
            return (result == 0) ? mid : ((result < 0) ? search(first, mid - 1, key) : search(mid + 1, last, key));
        }
        return -(first + 1);    // failed to find key
    }
    
    Vector<Pair> _list;
};

}
