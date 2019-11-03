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
#include <vector>
#include "Defines.h"

namespace m8r {

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
        Raw(RawType raw) : _raw(raw) { }
        RawType raw() const { return _raw; }

    private:
        RawType _raw;
    };
    
    typedef RawType value_type;
    
    Id() { _value = NoId; }
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
class ConstantId : public Id<uint8_t> { using Id::Id; };

//
//  Class: Map
//
//  Wrapper Map class that works on both Mac and ESP
//  Simle ordered array. Done this way to minimize space
//

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

//
//  Class: Stack
//
//  Wrapper around Vector to give stack semantics
//

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
    type* framePtr() { return (_frame < size()) ? &super::at(_frame) : nullptr; }
    uint32_t frame() const { return _frame; }
    
    const type& at(size_t i) const { return super::at(i); }
    type& at(size_t i) { return super::at(i); }

private:
    uint32_t _frame = 0;
};

//
//  Class: Pool
//
//  Object pool which stores values in multiple fixed size segments
//

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

//
//  Class: List
//
//  Singly linked list of ListItems
//

template<typename T, typename Key>
class List {
public:
    class Item {
        friend List;

    public:
        const Key& key() const { return _key; }

    private:
        T* _next = nullptr;
        Key _key;
    };

    template<typename IT>
    class Iterator {
    public:
        Iterator(IT* value = nullptr) : _value(value) { }
        
        operator bool() const { return _value; }
        Iterator<IT> operator++(T*){ auto t(*this); _value = _value->_next; return t; }
        Iterator<IT>& operator++(){ _value = _value->_next; return *this; }
        
    private:
        IT* _value;
    };
    
    using iterator = Iterator<List>;
    using const_iterator = Iterator<const List>;
    
    iterator begin() { return iterator(_head); }
    const_iterator begin() const { return const_iterator(_head); }
    
    iterator end() { return iterator(); }
    const_iterator end() const { return const_iterator(); }
    
    T* front() { return _head; }
    const T* front() const { return _head; }
    
    bool empty() const { return !_head; }
    
    void pop_front() { if (_head) _head = _head->_next; }

    void insert(T* newItem, Key key)
    {
        newItem->_next = nullptr;
        newItem->_key = key;
        T* prev = nullptr;
        
        for (T* item = _head; ; item = item->_next) {
            if (!item) {
                // Placing a new item in an empty list
                _head = newItem;
                break;
            }
            assert(item != newItem);
            if (key <= item->_key) {
                if (prev) {
                    // Placing a new item in a list that already has items
                    newItem->_next = prev->_next;
                    prev->_next = newItem;
                    return;
                } else {
                    // Placing a new item at the head of an existing list
                    newItem->_next = _head;
                    _head = newItem;
                    break;
                }
            }
            if (!item->_next) {
                // Placing new item at end of list
                item->_next = newItem;
                return;
            }
            prev = item;
        }
    }
    
    void remove(T* itemToRemove)
    {
        T* prev = nullptr;
        for (T* item = _head; item; item = item->_next) {
            if (item == itemToRemove) {
                if (!prev) {
                    _head = _head->_next;
                } else {
                    prev->_next = item->_next;
                }
                return;
            }
            prev = item;
        }
    }

private:
    T* _head = nullptr;
};

}
