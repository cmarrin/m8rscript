/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Defines.h"
#include <cstdlib>
#include <cstdint>

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Mallocator
//
//  C++11 Allocator which tracks allocations
//
//////////////////////////////////////////////////////////////////////////////

enum class MemoryType {
    Unknown,
    Object,
    String,
    Instruction,
    CallRecord,
    EventValue,
    ConstantValue,
    FunctionEntry,
    NumTypes
};
    
class MallocatorBase {
protected:
    static MallocatorBase* _list;
    
    MallocatorBase* _next;
    MemoryType _type;
    uint32_t _count = 0;
};

template <class T, MemoryType type = MemoryType::Unknown>
class Mallocator : public MallocatorBase {
public:
    typedef T value_type;
    Mallocator()
    {
        _type = type;
        _next = _list;
        _list = this;
    }
    
    template <class U> Mallocator(const Mallocator<U>&) : Mallocator() { }
    
    Mallocator(Mallocator&& other)
    {
        _type = other._type;
        _next = other._next;
        _count = other._count;
        if (_list == other) {
            _list = this;
        }
        other._next = nullptr;
    }
    
    T* allocate(std::size_t n)
    {
        _count++;
        return static_cast<T*>(m8r_malloc(n*sizeof(T)));
    }
    
    void deallocate(T* p, std::size_t)
    {
        _count--;
        m8r_free(p);
    }
    
    template <typename U>  
    struct rebind {
        typedef Mallocator<U> other;
    };
    
    static const MallocatorBase* list() { return _list; } 
};

template <class T, class U>
bool operator==(const Mallocator<T>&, const Mallocator<U>&) { return true; }

template <class T, class U>
bool operator!=(const Mallocator<T>&, const Mallocator<U>&) { return false; }

}
