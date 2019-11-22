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
#include "Mallocator.h"

namespace m8r {

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

//
//  Class: Vector
//
//  Vector class that works with the Mad allocator
//

template<typename T>
class Vector {
public:
    Vector() { }
    
    Vector(std::initializer_list<T> list) { insert(begin(), list.begin(), list.end()); }
    
    Vector(const Vector& other)
    {
        *this = other;
    };
    
    Vector(Vector&& other)
    {
        *this = other;
    }
    
    ~Vector() { _data.destroyVector(_capacity); }
    
    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() { return _size ? _data.get() : end(); }
    const_iterator begin() const { return _size ? _data.get() : end(); }
    iterator end() { return _data.get() + _size; }
    const_iterator end() const { return _data.get() + _size; }

    Vector& operator=(const Vector& other)
    {
        _data.destroyVector(_capacity);
        
        _data = Mad<T>();
        _size = 0;
        _capacity = 0;
        
        ensureCapacity(other._size);
        _size = other._size;
        
        for (int i = 0; i < _size; ++i) {
            _data.get()[i] = other._data.get()[i];
        }
        return *this;
    };

    Vector& operator=(Vector&& other)
    {
        _data.destroyVector(_capacity);

        _data = other._data;
        _size = other._size;
        _capacity = other._capacity;
        other._data = Mad<T>();
        other._size = 0;
        other._capacity = 0;
        return *this;
    };

    void push_back(T const &x)
    {
        ensureCapacity(_size + 1);
        new (_data.get() + _size) T();
        _data.get()[_size++] = x;
    };
    
    void pop_back()
    {
        _data.get()[_size - 1].~T();
        _size--;
    }
    
    template<class... Args>
    void emplace_back(Args&&... args)
    {
        push_back(T(args...));
    }
    
    void swap(Vector& other)
    {
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
        std::swap(_data, other._data);
    }
    
    bool empty() const { return _size == 0; }
    size_t size() const { return _size; };
    const T& operator[](uint16_t i) const { return at(i); };
    T& operator[](uint16_t i) { return at(i); };
    
    T& at(uint16_t i) { assert(i < _size); return _data.get()[i]; }
    const T& at(uint16_t i) const { assert(i < _size); return _data.get()[i]; }

    T& back() { return _data.get()[_size - 1]; }
    const T& back() const { return _data.get()[_size - 1]; }
    
    T& front() { return at(0); }
    const T& front() const { return at(0); }

    // TODO: Make this more robust and destroy erased element
    iterator erase(iterator pos) { return erase(pos, pos + 1); }
    
    iterator erase(iterator first, iterator last)
    {
        // end is one past the last element to erase
        if (first && first != end() && first < last) {
            if (last > end()) {
                last = end();
            }
            
            assert(_data.get() - first < _size);
            
            // destruct
            for (T* it = first; it != last; ++it) {
                it->~T();
            }
            
            uint32_t numToDelete = static_cast<uint32_t>(last - first);
            if (last < end()) {
                // move
                memmove(first, first + numToDelete, numToDelete * sizeof(T));
            }
            
            _size -= numToDelete;
        }
        return first;
    }
    
    iterator insert(iterator pos, const T& value)
    {
        return insert(pos, &value, (&value) + 1);
    }
    
    iterator insert(iterator pos, const_iterator from, const_iterator to)
    {
        uint16_t numToInsert = to - from;
        
        ensureCapacity(_size + numToInsert);
        if (_size) {
            memmove(pos + numToInsert, pos, _size - (pos - _data.get()) * sizeof(T));
        } else {
            // If _size == 0, set pos to the start of the vector
            pos = begin();
        }
        
        for (int i = 0; i < numToInsert; ++i) {
            new(pos + i) T();
            *(pos + i) = *(from + i);
        }
        
        _size += numToInsert;

        return pos;
    }
     
    void resize(uint16_t size)
    {
        if (size == _size) {
            return;
        }
        
        if (size > _size) {
            ensureCapacity(size);
            for (int i = _size; i < size; ++i) {
                new(_data.get() + i) T();
            }
            _size = size;
            return;
        }

        for (int i = size; i < _size; ++i) {
            _data.get()[i].~T();
        }
        _size = size;
    }
    
    void clear() { resize(0); }
    
    void reserve(uint16_t size) { ensureCapacity(size); }
    
private:
    void ensureCapacity(uint16_t size)
    {
        if (size <= _capacity) {
            return;
        }
        
        uint16_t oldCapacity = _capacity;
        
        _capacity = _capacity ? _capacity * 2 : 1;
        if (_capacity < size) {
            _capacity = size;
        }

        Mad<T> newData = Mad<T>::create(MemoryType::Vector, _capacity);
        for (int i = 0; i < _size; ++i) {
            new(&(newData.get()[i])) T();
            newData.get()[i] = _data.get()[i];
        }
        assert(_data.raw() != 1 && newData.raw() != 1);
        _data.destroyVector(oldCapacity);
        _data = newData;
    }

    uint16_t _size = 0;
    uint16_t _capacity = 0;
    Mad<T> _data;
};

//
//  Class: Stack
//
//  Wrapper around Vector to give stack semantics
//

template<typename T>
class Stack {
public:
    Stack() { }
    Stack(size_t reserveCount) { _stack.reserve(reserveCount); }
    
    void clear() { _stack.clear(); }
    size_t size() const { return _stack.size(); }
    void push(const T& value) { _stack.push_back(value); }
    void pop(size_t n = 1) { _stack.resize(size() - n); }
    void pop(T& value) { std::swap(value, top()); pop(); }

    T& top(int32_t relative = 0)
    {
        assert(static_cast<int32_t>(size()) + relative > 0);
        return at(static_cast<int32_t>(size()) + relative - 1);
    }
    
    const T& top(int32_t relative = 0) const
    {
        assert(static_cast<int32_t>(size()) + relative > 0);
        return at(static_cast<int32_t>(size()) + relative - 1);
    }
    
    void remove(int32_t relative)
    {
        _stack.erase(begin() + size() + relative - 1);
    }
    
    uint32_t setLocalFrame(uint32_t formalParams, uint32_t actualParams, uint32_t localSize)
    {
        uint32_t oldFrame = _frame;
        _frame = static_cast<uint32_t>(size()) - actualParams;
        uint32_t temps = localSize - formalParams;
        uint32_t extraParams = (formalParams > actualParams) ? (formalParams - actualParams) : 0;
        _stack.resize(size() + temps + extraParams);
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
    
    T& inFrame(int32_t index) { return _stack[_frame + index]; }
    const T& inFrame(int32_t index) const { return _stack[_frame + index]; }
    void setInFrame(int32_t index, const T& value) { _stack[_frame + index] = value; }
    void setTop(const T& value) { _stack.back() = value; }
    bool empty() const { return _stack.empty(); }

    const T* framePtr() const { return &at(_frame); }
    T* framePtr() { return (_frame < size()) ? &at(_frame) : nullptr; }
    uint32_t frame() const { return _frame; }
    
    const T& at(size_t i) const { return _stack[i]; }
    T& at(size_t i) { return _stack[i]; }
    
    T* begin() { return _stack.begin(); }
    const T* begin() const { return _stack.begin(); }
    
    T* end() { return _stack.end(); }
    const T* end() const { return _stack.end(); }

private:
    Vector<T> _stack;
    uint32_t _frame = 0;
};

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
    
    using MapList = Vector<Pair>;
    using iterator = typename MapList::iterator;
    using const_iterator = typename MapList::const_iterator;

    const_iterator find(const Key& key) const
    {
        int result = search(0, static_cast<int>(_list.size()) - 1, key);
        if (result < 0) {
            return _list.end();
        }
        return _list.begin() + result;
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
    
    MapList _list;
    Compare _compare;
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
    
    Vector<Segment> _segments;
    Entry* _freeList = nullptr;
};

}
