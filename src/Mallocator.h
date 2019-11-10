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

//---------------------------------------------------------------------------
//
//  Class: Mallocator
//
//  C++11 Allocator which tracks allocations
//
//  Design of the allocator
//
//  This allocator has zero overhead for allocated storage and allocates
//  on 4 byte boundariess. This is problematic on Mac, which expects to
//  align pointers to 8 byte boundaries. So some adjustment may be needed
//  But in a low memory envoronment (~50KB) it's crucial to save space, even
//  if it means taking more time to, for instance, free a block.
//
//  Memory is alloted on 4 byte boundaries. Memory is tracked through block
//  id, which is a uint16_t. With 4 bytes blocks, that allows 256KB of memory.
//  For larger memory sizes the block size can be increased. 8 byte blocks
//  would allow 512KB and 16 byte blocks would allow 1MB. Larger than that
//  maybe this is not the right allocator. But for the ESP8266 at around 
//  50KB of usable RAM and the ESP32 at around 400KB, this allocator should
//  do fine.
//
//  The block id allows blocks to be on 4 byte boundaries, but blocks can be
//  larger than that. Allocated blocks can be as large as available memory.
//
//  The free list uses the first 4 bytes of a block, 2 bytes for a block id
//  of the next free block, and a 2 byte block size. Actually the size byte
//  may contain some flags, so the maximum free block size may be smaller.
//  If so, we will have to allow for contiguous free blocks. While possible
//  that complicates the design and require coalescing free blocks. So maybe
//  we can limit the maximum allocated block size to 14 bits, 16K blocks or 
//  64KB. That gives 2 bits of flags, if needed.
//
//---------------------------------------------------------------------------

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

class MaterObject;
class String;

using RawMad = uint16_t;

template<typename T>
class Mad : public Id<RawMad>
{
public:
    Mad() : Id()
    {
#ifndef NDEBUG
        _ptr = nullptr;
#endif
    }
    
    Mad(Raw raw) : Id(raw)
    {
#ifndef NDEBUG
        _ptr = get();
#endif
    }

    explicit Mad(const T*);
    
    T* get() const;
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }
    
    template <typename U>
    operator Mad<U>() const
    {
        // Verify that U and T are either the same class or T is a subclass of U
        U* u = nullptr;
        const T* t = static_cast<const T*>(u);
        (void) t;

        return Mad<U>(raw());
    }
    
    void reset() { *this = Mad<T>(); }
    
    void destroy(size_t size = 1);
    
    static Mad<T> create();
    static Mad<String> create(const char*, int32_t length = -1);
    static MemoryType type() { return MemoryType::Unknown; }
    
private:
#ifndef NDEBUG
    // Keep a pointer around for debugging
    T* _ptr = nullptr;
#endif
};    

class Mallocator
{
public:
    template<typename T>
    Mad<T> allocate(size_t size)
    {
        uint32_t index = static_cast<uint32_t>(Mad<T>::type());
        _list[index].count++;
        _list[index].size += size;

        assert(static_cast<uint32_t>(size) <= 0xffff);
        return Mad<T>(alloc(size));
    }
    
    template<typename T>
    void deallocate(Mad<T> p, size_t size)
    {
        uint32_t index = static_cast<uint32_t>(Mad<T>::type());
        _list[index].count--;
        _list[index].size -= size;

        free(p.raw(), size);
    }
    
    struct Entry
    {
        uint32_t size = 0;
        uint32_t count = 0;
    };
    
    const Entry& entry(MemoryType which) { return _list[static_cast<uint32_t>(which)]; }
    
    static Mallocator* shared()
    {
        if (!_sharedMallocator) {
            _sharedMallocator = new Mallocator();
        }
        return _sharedMallocator;
    }

    void* get(RawMad p) const { return (p < _heapBlockCount) ? (_heapBase + p * _blockSize) : nullptr; }
    
    RawMad blockIdFromAddr(void* addr)
    {
        // return NoId unless the address is in the heap range AND is divisible by _blockSize
        if (addr < _heapBase || addr > (_heapBase + _heapBlockCount * _blockSize)) {
            return Id<RawMad>();
        }
        
        size_t offset = reinterpret_cast<char*>(addr) -_heapBase;
        if ((offset % _blockSize) != 0) {
            return Id<RawMad>();
        }
        
        return static_cast<RawMad>(offset / _blockSize); 
    }

protected:
    Entry _list[static_cast<uint32_t>(MemoryType::NumTypes)];

private:
    Mallocator();
    
    RawMad alloc(size_t size);
    void free(RawMad, size_t size);

    using BlockId = RawMad;
    using SizeInBlocks = uint16_t;
    
    static constexpr BlockId NoBlockId = static_cast<BlockId>(-1);

    struct FreeHeader
    {
        BlockId nextBlock;
        SizeInBlocks sizeInBlocks;
    };
    
    FreeHeader* block(BlockId b)
    {
        return reinterpret_cast<FreeHeader*>(_heapBase + (b * _blockSize));
    }
    
    char* _heapBase = nullptr;
    SizeInBlocks _heapBlockCount = 0;
    uint16_t _blockSize = 4;
    BlockId _firstFreeBlock = 0;
    
    static Mallocator* _sharedMallocator;
};

template<typename T>
Mad<T>::Mad(const T* addr)
{
    *this = Mad(Mad::Raw(Mallocator::shared()->blockIdFromAddr(const_cast<T*>(addr))));
}

template<typename T>
inline T* Mad<T>::get() const { return reinterpret_cast<T*>(Mallocator::shared()->get(raw())); }

template<typename T>
inline void Mad<T>::destroy(size_t size)
{
    if (*this) {
        get()->~T();
        Mallocator::shared()->deallocate(*this, sizeof(T) * size);
    }
}

template<typename T>
inline Mad<T> Mad<T>::create()
{
    Mad<T> obj = Mallocator::shared()->allocate<T>(sizeof(T));
    new(obj.get()) T();
    return obj;
}

template<>
Mad<String> Mad<String>::create(const char* s, int32_t length);

template<> inline MemoryType Mad<Instruction>::type()   { return MemoryType::Instruction; }

//class MallocatorBase
//{
//public:
//    struct Entry
//    {
//        uint32_t size = 0;
//        uint32_t count = 0;
//    };
//
//    static const Entry& entry(MemoryType which) { return _list[static_cast<uint32_t>(which)]; }
//
//    static void* allocate(MemoryType which, size_t size)
//    {
//        _list[static_cast<uint32_t>(which)].count++;
//        _list[static_cast<uint32_t>(which)].size += size;
//        return m8r_malloc(size);
//    }
//
//    static void deallocate(MemoryType which, void* p, size_t size)
//    {
//        _list[static_cast<uint32_t>(which)].count--;
//        _list[static_cast<uint32_t>(which)].size -= size;
//        m8r_free(p);
//    }
//
//protected:
//    static Entry _list[static_cast<uint32_t>(MemoryType::NumTypes)] ;
//};
//

}
