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
        return static_cast<T*>(std::malloc(n*sizeof(T)));
    }
    
    void deallocate(T* p, std::size_t)
    {
        _count--;
        std::free(p);
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
