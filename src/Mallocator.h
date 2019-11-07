/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
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
    
class MallocatorBase
{
public:
    struct Entry
    {
        uint32_t size = 0;
        uint32_t count = 0;
    };
    
    static const Entry& entry(MemoryType which) { return _list[static_cast<uint32_t>(which)]; }
    
    static void* allocate(MemoryType which, size_t size)
    {
        _list[static_cast<uint32_t>(which)].count++;
        _list[static_cast<uint32_t>(which)].size += size;
        return m8r_malloc(size);
    }
    
    static void deallocate(MemoryType which, void* p, size_t size)
    {
        _list[static_cast<uint32_t>(which)].count--;
        _list[static_cast<uint32_t>(which)].size -= size;
        m8r_free(p);
    }
    
protected:
    static Entry _list[static_cast<uint32_t>(MemoryType::NumTypes)] ;
};

template <class T, MemoryType Type = MemoryType::Unknown>
class Mallocator : public MallocatorBase {
public:
    typedef T value_type;

    T* allocate(std::size_t n)
    {
        _list[static_cast<uint32_t>(Type)].count++;
        _list[static_cast<uint32_t>(Type)].size += n * sizeof(T);
        return static_cast<T*>(m8r_malloc(n * sizeof(T)));
    }
    
    void deallocate(T* p, std::size_t n)
    {
        _list[static_cast<uint32_t>(Type)].count--;
        _list[static_cast<uint32_t>(Type)].size -= n;
        m8r_free(p);
    }
    
    template <typename U>  
    struct rebind {
        typedef Mallocator<U> other;
    };
};

template <class T, class U>
bool operator==(const Mallocator<T>&, const Mallocator<U>&) { return true; }

template <class T, class U>
bool operator!=(const Mallocator<T>&, const Mallocator<U>&) { return false; }

}
