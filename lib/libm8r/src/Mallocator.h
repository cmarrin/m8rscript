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
#include <typeinfo>

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
//  If MemoryType is Fixed it means this block is being allocated by a
//  traditional malloc style call. The corresponding free() call will
//  not pass the size, so we need to store it in a header. We will make
//  space in this header for a BlockId size value so we can make a linked
//  list of these block for when we want to find them for compaction
//
//---------------------------------------------------------------------------

// Memory header for all memory blocks.
//
// Headers are 4 uint16_t:
//
//      magic: the value 0xbeef to verify memory has not be stomped on
//      type: allocated or free
//      next: used only for free blocks at this point
//      size: size in blocks
//
// This piggybacks on the header used for free blocks and Fixed allocations. It
// add magic and type and is put at the head of all allocated objects in addition
// to free and fixed blocks.

#ifndef NDEBUG
#define DEBUG_MEMORY_HEADER
#endif

using RawMad = intptr_t;
static constexpr RawMad NoRawMad = 0;

struct MemoryInfo{
    struct Entry
    {
        uint32_t size = 0;
        uint32_t count = 0;
    };
    
    uint32_t totalAllocatedBytes = 0;
    uint16_t numAllocations = 0;
    std::array<Entry, static_cast<uint32_t>(MemoryType::NumTypes)> allocationsByType;
};

template<typename T>
class Mad
{
public:
    Mad() { }
    
    explicit Mad(RawMad raw) : _raw(reinterpret_cast<T*>(raw)) { }
    
    explicit Mad(const T* addr) { _raw = const_cast<T*>(addr); }
    
    Mad(const Mad& other) { *this = other; }
    
    RawMad raw() const { return reinterpret_cast<RawMad>(_raw); }

    T* get() const { return reinterpret_cast<T*>(_raw); }
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }

    bool operator==(const Mad& other) const { return _raw == other._raw; }

    bool valid() const { return _raw != nullptr; }
    
    template <typename U>
    operator Mad<U>() const
    {
        // Verify that U and T are either the same class or T is a subclass of U
        T* t = nullptr;
        const U* u = t;
        (void) u;

        return Mad<U>(raw());
    }
    
    void reset() { *this = Mad<T>(); }
    
    template<typename X>
    struct assert_false : std::false_type { };
    void destroy() { destroy(MemoryType::Unknown); }
    
    void destroyVector() { destroyHelper(MemoryType::Vector, false); }
    void destroy(MemoryType type) { destroyHelper(type, true); }

    static Mad<T> create(MemoryType, uint16_t n = 1);
    
    static Mad<T> create(uint16_t n) { static_assert(assert_false<T>::value, "Must specialize this function"); return Mad<T>(); }
    static Mad<T> create() { return create(T::memoryType(), 1); }

private:
    T* _raw = nullptr;
    
    void destroyHelper(MemoryType, bool destruct);
};    

class Mallocator
{
public:
    template<typename T>
    Mad<T> allocate(MemoryType type, uint16_t nElements)
    {
        return Mad<T>(alloc(static_cast<uint32_t>(nElements) * sizeof(T), type));
    }
    
    template<typename T>
    void deallocate(MemoryType type, Mad<T> p)
    {
        free(p.raw(), type);
    }
    
    static Mallocator* shared() { return &_mallocator; }

    const MemoryInfo& memoryInfo() const { return _memoryInfo; }
    
    uint32_t freeSize() const;

    static const char* stringFromMemoryType(MemoryType);

protected:
    MemoryInfo _memoryInfo;

private:
    RawMad alloc(uint32_t size, MemoryType type);
    void free(RawMad, MemoryType type);
        
    static Mallocator _mallocator;

};

template<typename T>
inline void Mad<T>::destroyHelper(MemoryType type, bool destruct)
{
    if (!valid()) {
        return;
    }
    
    if (this->valid()) {
        // Only call the destructor if this isn't an array of objects.
        // Arrays call their destructors themselves
        if (destruct) {
            get()->~T();
        }
        Mallocator::shared()->deallocate(type, *this);
    }
}

template<typename T>
inline Mad<T> Mad<T>::create(MemoryType type, uint16_t n)
{
    Mad<T> obj = Mallocator::shared()->allocate<T>(type, n);
    
    // Only call the constructor if this isn't an array of objects.
    // Arrays call their consctructors themselves
    if (n == 1) {
        new(obj.get()) T();
    }
    return obj;
}

template<>
inline Mad<char> Mad<char>::create(uint16_t n)
{
    Mad<char> p = Mallocator::shared()->allocate<char>(MemoryType::Character, n);
    return p;
}

template<>
inline Mad<uint8_t> Mad<uint8_t>::create(uint16_t n)
{
    return Mallocator::shared()->allocate<uint8_t>(MemoryType::Character, n);
}

template<>
inline void Mad<char>::destroy()
{
    if (!valid()) {
        return;
    }
    Mallocator::shared()->deallocate(MemoryType::Character, *this);
}

template<>
inline void Mad<uint8_t>::destroy()
{
    if (!valid()) {
        return;
    }
    Mallocator::shared()->deallocate(MemoryType::Character, *this);
}

}
