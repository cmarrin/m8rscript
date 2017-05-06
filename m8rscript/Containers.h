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
#include <vector>
#include "Defines.h"

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Id/RawId template
//
//  Generic Id class
//
//////////////////////////////////////////////////////////////////////////////

template <typename RawType>
class Id
{
public:
    class Raw
    {
        friend class Id;

    public:
        Raw() : _raw(NoId) { }
        Raw(RawType raw) : _raw(raw) { }
        RawType raw() const { return _raw; }

    private:
        RawType _raw;
    };
    
    typedef RawType value_type;
    
    Id() { }
    Id(Raw raw) { _value._raw = raw._raw; }
    //Id(RawType raw) { _value._raw = raw; }
    Id(const Id& other) { _value._raw = other._value._raw; }
    Id(Id&& other) { _value._raw = other._value._raw; }

    RawType raw() const { return _value._raw; }

    const Id& operator=(const Id& other) { _value._raw = other._value._raw; return *this; }
    Id& operator=(Id& other) { _value._raw = other._value._raw; return *this; }
    operator bool() const { return _value._raw != NoId; }
    operator Raw() const { return _value; }

    int operator-(const Id& other) const { return static_cast<int>(_value._raw) - static_cast<int>(other._value._raw); }
    bool operator==(const Id& other) const { return _value._raw == other._value._raw; }

private:
    static constexpr RawType NoId = std::numeric_limits<RawType>::max();

    Raw _value;
};

class StringLiteral : public Id<uint32_t> { using Id::Id; };
class Atom : public Id<uint16_t> { using Id::Id; };
class ConstantId : public Id<uint8_t> { using Id::Id; };

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
        if (!s) {
            return;
        }
        if (len == -1) {
            len = static_cast<int32_t>(strlen(s));
        }
        ensureCapacity(len + 1);
        if (len) {
            memcpy(_data, s, len);
        }
        _size = len + 1;
        _data[_size - 1] = '\0';
    }
    
    String(const String& other) : _data(nullptr)
    {
        *this = other;
    };
    
    ~String() { delete [ ] _data; };

    String& operator=(const String& other)
    {
        if (_data) {
            free(_data);
        }
        _size = other._size;
        _capacity = other._capacity;
        if (!other._data) {
            _data = nullptr;
            return *this;
        }
        
        _data = new char[_capacity];
        assert(_data);
        if (_data) {
            memcpy(_data, other._data, _size);
        } else {
            _capacity = 0;
            _size = 1;
        }
        return *this;
    }
    
    const char& operator[](size_t i) const { assert(i >= 0 && i < _size - 1); return _data[i]; };
    char& operator[](size_t i) { assert(i >= 0 && i < _size - 1); return _data[i]; };
	size_t size() const { return _size ? (_size - 1) : 0; }
    bool empty() const { return _size <= 1; }
    void clear() { _size = 1; if (_data) _data[0] = '\0'; }
	String& operator+=(uint8_t c)
    {
        ensureCapacity(_size + 1);
        _data[_size - 1] = c;
        _data[_size++] = '\0';
        return *this;
    }
    
	String& operator+=(char c)
    {
        ensureCapacity(_size + 1);
        _data[_size - 1] = c;
        _data[_size] = '\0';
        _size += 1;
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
    
    friend String operator +(const String& s1 , const String& s2) { String s = s1; s += s2; return s; }
    friend String operator +(const String& s1 , const char* s2) { String s = s1; s += s2; return s; }
    friend String operator +(const char* s1 , const String& s2) { String s = s1; s += s2; return s; }
    
    bool operator<(const String& other) const { return strcmp(c_str(), other.c_str()) < 0; }
    bool operator==(const String& other) const { return strcmp(c_str(), other.c_str()) == 0; }
    bool operator!=(const String& other) const { return strcmp(c_str(), other.c_str()) != 0; }

    const char* c_str() const { return _data ? _data : ""; }
    String& erase(size_t pos, size_t len)
    {
        if (pos >= _size - 1) {
            return *this;
        }
        if (pos + len >= _size) {
            len = _size - pos - 1;
        }
        memmove(_data + pos, _data + pos + len, _size - pos - len);
        _size -= len;
        return *this;
    }

    String& erase(size_t pos = 0)
    {
        return erase(pos, _size - pos);
    }
    
    String slice(int32_t start, int32_t end) const
    {
        int32_t sz = static_cast<int32_t>(size());
        if (start < 0) {
            start = sz + start;
        }
        if (end < 0) {
            end = sz + end;
        }
        if (end > sz) {
            end = sz;
        }
        if (start >= end) {
            return String();
        }
        return String(_data + start, end - start);
    }
    
    String slice(int32_t start) const
    {
        return slice(start, static_cast<int32_t>(size()));
    }
    
    String trim() const
    {
        if (_size < 2 || !_data) {
            return String();
        }
        size_t l = _size - 1;
        char* s = _data;
        while (isspace(s[l - 1])) {
            --l;
        }
        while (*s && isspace(*s)) {
            ++s;
            --l;
        }
        return String(s, static_cast<int32_t>(l));
    }
    
    // If skipEmpty is true, substrings of zero length are not added to the array
    std::vector<String> split(const String& separator, bool skipEmpty = false) const
    {
        std::vector<String> array;
        char* p = _data;
        while (1) {
            char* n = strstr(p, separator.c_str());
            if (!n || n - p != 0 || !skipEmpty) {
                array.push_back(String(p, static_cast<int32_t>(n ? (n - p) : -1)));
            }
            if (!n) {
                break;
            }
            p = n ? (n + separator.size()) : nullptr;
        }
        return array;
    }
    
    static String join(const std::vector<String>& array, const String& separator)
    {
        String s;
        bool first = true;
        for (auto it : array) {
            if (first) {
                first = false;
            } else {
                s += separator;
            }
            s += it;
        }
        return s;
    }
    
    bool isMarked() const { return _marked; }
    void setMarked(bool b) { _marked = b; }
	
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
        char *newData = new char[_capacity];
        assert(newData);
        if (_data) {
            if (newData) {
                memcpy(newData, _data, _size);
            } else {
                _capacity = 0;
                _size = 1;
            }
            delete _data;
        }
        _data = newData;
    };

    size_t _size;
    size_t _capacity;
    char *_data;
    bool _marked = true;
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
    int operator()(const T& lhs, const T& rhs) const { return static_cast<int>(lhs - rhs); }
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
    
    typedef std::vector<Pair> List;
    typedef typename List::iterator iterator;
    typedef typename List::const_iterator const_iterator;

    const_iterator find(const Key& key) const
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        if (result < 0) {
            return _list.cend();
        }
        return _list.cbegin() + result;
    }
    
    iterator find(const Key& key)
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        if (result < 0) {
            return _list.end();
        }
        return _list.begin() + result;
    }
    
    std::pair<iterator, bool> emplace(const Key& key, const Value& value)
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        bool placed = false;
        if (result < 0) {
            _list.resize(_list.size() + 1);
            int sizeToMove = static_cast<int>(_list.size()) + result;
            if (sizeToMove) {
                memmove(&_list[-result], &_list[-result - 1], sizeToMove * sizeof(Pair));
            }
            result = -result - 1;
            _list[result] = { key, value };
            placed = true;
        }
        return { begin() + result, placed };
    }
    
    iterator begin() { return _list.begin(); }
    const_iterator begin() const { return _list.begin(); }
    iterator end() { return _list.end(); }
    const_iterator end() const { return _list.end(); }
    
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
    
    List _list;
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
class Stack : std::vector<type> {
    typedef std::vector<type> super;
    
public:
    using std::vector<type>::begin;
    using std::vector<type>::end;
    
    Stack() { }
    Stack(size_t reserveCount) { super::reserve(reserveCount); }
    
    void clear() { super::clear(); }
    size_t size() const { return super::size(); }
    void push(const type& value) { super::push_back(value); }
    void pop(size_t n = 1) { super::resize(size() - n); }
    void pop(type& value) { std::swap(value, top()); pop(); }

    type& top(int32_t relative = 0)
    {
        assert(static_cast<int32_t>(super::size()) + relative > 0);
        return super::at(static_cast<int32_t>(super::size()) + relative - 1);
    }
    
    const type& top(int32_t relative = 0) const
    {
        assert(static_cast<int32_t>(super::size()) + relative > 0);
        return super::at(static_cast<int32_t>(super::size()) + relative - 1);
    }
    
    void remove(int32_t relative)
    {
        super::erase(super::begin() + super::size() + relative - 1);
    }
    
    uint32_t setLocalFrame(uint32_t formalParams, uint32_t actualParams, uint32_t localSize)
    {
        uint32_t oldFrame = _frame;
        _frame = static_cast<uint32_t>(size()) - actualParams;
        uint32_t temps = localSize - formalParams;
        uint32_t extraParams = (formalParams > actualParams) ? (formalParams - actualParams) : 0;
        super::resize(size() + temps + extraParams);
        return oldFrame;
    }
    void restoreFrame(uint32_t frame, uint32_t localsToPop)
    {
        assert(frame <= size() && frame <= _frame);
        pop(localsToPop);
        _frame = frame;
    }
    
    // Ensure that current frame equals passed frame and the the stack has localSize values on it
    bool validateFrame(size_t frame, size_t localSize)
    {
        return frame == _frame && size() == localSize;
    }
    
    type& inFrame(int32_t index) { return super::at(_frame + index); }
    const type& inFrame(int32_t index) const { return super::at(_frame + index); }
    void setInFrame(int32_t index, const type& value) { super::at(_frame + index) = value; }
    void setTop(const type& value) { super::back() = value; }
    bool empty() const { return super::empty(); }

    const type* framePtr() const { return &super::at(_frame); }
    type* framePtr() { return size() ? &super::at(_frame) : nullptr; }
    uint32_t frame() const { return _frame; }
    
    const type& at(size_t i) const { return super::at(i); }
    type& at(size_t i) { return super::at(i); }

private:
    uint32_t _frame = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Pool
//
//  Object pool which stores values in multiple fixed size segments
//
//////////////////////////////////////////////////////////////////////////////

template<typename type, uint32_t numPerSegment>
class Pool {
public:
    type* alloc()
    {
        if (!_freeList) {
            allocSegment();
        }
        Entry* entry = _freeList;
        _freeList = _freeList->next();
        return entry->value();
    }
            
    void free(type* value)
    {
        Entry* entry = reinterpret_cast<Entry*>(value);
        entry->setNext(_freeList);
        _freeList = entry;
    }

private:
    void allocSegment()
    {
        assert(!_freeList);
        _segments.resize(_segments.size() + 1);
        Entry prev = nullptr;
        for (auto it : _segments.back()) {
            if (!_freeList) {
                _freeList = it;
            } else {
                assert(prev);
                prev->setNext(it);
            }
            it->setNext(nullptr);
            prev = it;
        }
    }
    
    class Entry {
    public:
        const type* value() const { return reinterpret_cast<type*>(_value); }
        type* value() { return reinterpret_cast<type*>(_value); }
        Entry* next() const { return reinterpret_cast<Entry*>(_value); }
        void setNext(Entry* next) { reinterpret_cast<Entry*>(_value) = next; }
        
    private:
        static constexpr uint32_t size = std::max(sizeof(type), sizeof(void*));
        uint8_t _value[size];
    };
    
    typedef Entry Segment[numPerSegment];
    
    std::vector<Segment> _segments;
    Entry* _freeList = nullptr;
};

}
