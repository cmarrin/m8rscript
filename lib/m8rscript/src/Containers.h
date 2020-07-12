/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <algorithm>
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
    
    ~Vector()
    {
        clear();
        _data.destroyVector();
    }
    
    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() { return _size ? _data.get() : end(); }
    const_iterator begin() const { return _size ? _data.get() : end(); }
    iterator end() { return _data.get() + _size; }
    const_iterator end() const { return _data.get() + _size; }

    Vector& operator=(const Vector& other)
    {
        clear();
        _data.destroyVector();
        
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
        clear();
        _data.destroyVector();

        _data = other._data;
        _size = other._size;
        _capacity = other._capacity;
        other._data = Mad<T>();
        other._size = 0;
        other._capacity = 0;
        return *this;
    };
    
    void assign(const_iterator first, const_iterator last)
    {
        clear();
        reserve(last - first);
        std::copy(first, last, begin());
        _size = last - first;
    }

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
    iterator erase(iterator pos)
    {
        if (pos == end()) {
            return end();
        }
        
        return erase(pos, pos + 1);
    }
    
    iterator erase(iterator first, iterator last)
    {
        if (first == end()) {
            return end();
        }
        
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
                memmove(first, first + numToDelete, (_size - numToDelete) * sizeof(T));
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
    
    bool remove(const T& element)
    {
        auto it = std::find(begin(), end(), element);
        if (it == end()) {
            return false;
        }
        erase(it);
        return true;
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
        
        _capacity = _capacity ? _capacity * 2 : 1;
        if (_capacity < size) {
            _capacity = size;
        }

        Mad<T> newData = Mad<T>::create(MemoryType::Vector, _capacity);
        for (int i = 0; i < _size; ++i) {
            new(&(newData.get()[i])) T();
            newData.get()[i] = _data.get()[i];
            _data.get()[i].~T();
        }
        assert(_data.raw() != 1 && newData.raw() != 1);
        _data.destroyVector();
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
    
    // This assumes the stack has actualParams pushed. So the new frame will be
    // at the current stack position minus actualParams. And the new stack position
    // will be after the formalParams plus the localSize. If actualParams is 
    // less than formalParams, the difference will be pushed onto the stack as
    // undefined values. So on exit, the values between the new frame and TOS will
    // be actualParams, any extra params if formal > actual, and locals. Return
    // the previous frame and number of values added to the stack so we can 
    // pass these values to restoreFrame.
    void setLocalFrame(uint32_t formalParams, uint32_t actualParams, uint32_t localSize, uint32_t& prevFrame, uint32_t& localsAdded)
    {
        prevFrame = _frame;
        _frame = static_cast<uint32_t>(size()) - actualParams;
        uint32_t temps = localSize - formalParams;
        uint32_t extraParams = (formalParams > actualParams) ? (formalParams - actualParams) : 0;
        localsAdded = temps + extraParams;
        _stack.resize(size() + localsAdded);
    }
    
    void setLocalFrame(uint32_t formalParams, uint32_t actualParams, uint32_t localSize)
    {
        uint32_t prevFrame;
        uint32_t localsAdded;
        setLocalFrame(formalParams, actualParams, localSize, prevFrame, localsAdded);
    }
    
    // Restore the frame to the passed value (presumably the value returned
    // from a previous call to setLocalFrame). And pop the pass number of
    // locals, which should restore the stack to the 
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

    uint32_t frame() const { return _frame; }

    const T& atFrame(size_t i) const { return at(_frame + i); }
    T& atFrame(size_t i) { return at(_frame + i); }

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

template<typename Key, typename Value>
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

    Pair& back() { return _list.back(); }
    const Pair& back() const { return _list.back(); }
    
    Pair& front() { return _list.front(); }
    const Pair& front() const { return _list.front(); }

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
    
    iterator erase(iterator it) { return _list.erase(it); }
    
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
            int result = compare(key, _list[mid].key);
            return (result == 0) ? mid : ((result < 0) ? search(first, mid - 1, key) : search(mid + 1, last, key));
        }
        return -(first + 1);    // failed to find key
    }
    
    MapList _list;
};

}
