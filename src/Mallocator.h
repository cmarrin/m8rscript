/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

//#include "Containers.h"
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
    
    Mad(const Mad& other)
    {
        *this = other;
#ifndef NDEBUG
        _ptr = get();
#endif
    }
    
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

        return Mad<U>(Raw(raw()));
    }
    
    void reset() { *this = Mad<T>(); }
    
    void destroy(size_t size = 1);
    
    static Mad<T> create(uint32_t n = 1);
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
        assert(Mad<T>::type() != MemoryType::Unknown);
        assert(static_cast<uint32_t>(size) <= 0xffff);
        
        uint16_t sizeBefore = _memoryInfo.freeSizeInBlocks;
        Mad<T> result = Mad<T>(Id<RawMad>::Raw(alloc(size)));
        
        if (result) {
            uint32_t index = static_cast<uint32_t>(Mad<T>::type());
            _memoryInfo.allocationsByType[index].count++;
            _memoryInfo.allocationsByType[index].sizeInBlocks += sizeBefore - _memoryInfo.freeSizeInBlocks;
        }
        return result;
    }
    
    template<typename T>
    void deallocate(Mad<T> p, size_t size)
    {
        assert(Mad<T>::type() != MemoryType::Unknown);
        uint16_t sizeBefore = _memoryInfo.freeSizeInBlocks;
        free(p.raw(), size);

        uint32_t index = static_cast<uint32_t>(Mad<T>::type());
        _memoryInfo.allocationsByType[index].count--;
        _memoryInfo.allocationsByType[index].sizeInBlocks -= _memoryInfo.freeSizeInBlocks - sizeBefore;
    }
    
    static Mallocator* shared() { return &_mallocator; }

    void* get(RawMad p) const
    {
        return (p < _memoryInfo.heapSizeInBlocks) ? (_heapBase + p * _memoryInfo.blockSize) : nullptr;
    }
    
    RawMad blockIdFromAddr(void* addr)
    {
        // return NoId unless the address is in the heap range AND is divisible by _blockSize
        if (addr < _heapBase || addr > (_heapBase + _memoryInfo.heapSizeInBlocks * _memoryInfo.blockSize)) {
            return Id<RawMad>().raw();
        }
        
        size_t offset = reinterpret_cast<char*>(addr) -_heapBase;
        if ((offset % _memoryInfo.blockSize) != 0) {
            return Id<RawMad>().raw();
        }
        
        return static_cast<RawMad>(offset / _memoryInfo.blockSize);
    }
    
    const MemoryInfo& memoryInfo() const { return _memoryInfo; }

protected:
    MemoryInfo _memoryInfo;

private:
    Mallocator();
    
    using BlockId = RawMad;

    RawMad alloc(size_t size);
    void free(RawMad, size_t size);
    
    void coalesce(BlockId prev, BlockId next);

    uint16_t blockSizeFromByteSize(size_t size) { return (size + _memoryInfo.blockSize - 1) / _memoryInfo.blockSize; }
    
    static constexpr BlockId NoBlockId = static_cast<BlockId>(-1);

    struct FreeHeader
    {
        BlockId nextBlock;
        uint16_t sizeInBlocks;
    };
    
    FreeHeader* block(BlockId b)
    {
        return reinterpret_cast<FreeHeader*>(_heapBase + (b * _memoryInfo.blockSize));
    }
    
    char* _heapBase = nullptr;
    BlockId _firstFreeBlock = 0;

    static Mallocator _mallocator;
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
    if (size == 0) {
        return;
    }
    
    if (*this) {
        // Only call the destructor if this isn't an array of objects.
        // Arrays call their desctructors themselves
        if (size == 1) {
            get()->~T();
        }
        Mallocator::shared()->deallocate(*this, sizeof(T) * size);
    }
}

template<typename T>
inline Mad<T> Mad<T>::create(uint32_t n)
{
    Mad<T> obj = Mallocator::shared()->allocate<T>(sizeof(T) * n);
    
    // Only call the constructor if this isn't an array of objects.
    // Arrays call their consctructors themselves
    if (n == 1) {
        new(obj.get()) T();
    }
    return obj;
}

template<>
Mad<String> Mad<String>::create(const char* s, int32_t length);

template<> inline MemoryType Mad<Instruction>::type() { return MemoryType::Instruction; }
template<> inline MemoryType Mad<char>::type() { return MemoryType::Character; }

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
