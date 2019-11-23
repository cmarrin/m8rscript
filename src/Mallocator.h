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
#include "GC.h"
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
//  If MemoryType is Fixed it means this block is being allocated by a
//  traditional malloc style call. The corresponding free() call will
//  not pass the size, so we need to store it in a header. We will make
//  space in this header for a BlockId size value so we can make a linked
//  list of these block for when we want to find them for compaction
//
//---------------------------------------------------------------------------

class NativeObject;
class Object;
class String;

#ifndef NDEBUG
#define MEMORY_PTR
#endif

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
#define MEMORY_HEADER
#endif

template<typename T>
class Mad
{
public:
    Mad()
    {
#ifdef MEMORY_PTR
        _ptr = nullptr;
#endif
    }
    
    explicit Mad(RawMad raw) : _raw(raw)
    {
#ifdef MEMORY_PTR
        _ptr = get();
#endif
    }
    
    explicit Mad(const T*);
    
    Mad(const Mad& other)
    {
        *this = other;
#ifdef MEMORY_PTR
        _ptr = get();
#endif
    }
    
    RawMad raw() const { return _raw; }

    T* get() const;
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }

    bool operator==(const Mad& other) const { return _raw == other._raw; }

    bool valid() const { return _raw != Mad<T>().raw(); }
    
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
    
    template<typename X>
    struct assert_false : std::false_type { };
    void destroy(uint16_t n) { static_assert(assert_false<T>::value, "Must specialize this function"); }
    
    void destroyVector(uint32_t n) { destroyHelper(MemoryType::Vector, n * sizeof(T), false); }
    void destroy(MemoryType type) { destroyHelper(type, sizeof(T), true); }
    void destroy(MemoryType type, uint16_t size) { destroyHelper(type, size, true); }
    void destroy() { destroyHelper(T::memoryType(), sizeof(T), true); }

    static Mad<T> create(MemoryType, uint16_t n = 1);
    static Mad<T> create(uint16_t n) { return create(MemoryType::Unknown, n); }
    static Mad<T> create() { return create(T::memoryType(), 1); }
    static Mad<String> create(const String& s);
    static Mad<String> create(String&& s);
    static Mad<String> create(const char*, int32_t length = -1);

private:
    RawMad _raw = NoRawMad;
    
    void destroyHelper(MemoryType, uint16_t size, bool destruct);
    
#ifdef MEMORY_PTR
    // Keep a pointer around for debugging
    T* _ptr = nullptr;
#endif
};    

class Mallocator
{
public:
    void init();
    
    template<typename T>
    Mad<T> allocate(MemoryType type, size_t size)
    {
        return Mad<T>(alloc(size, type));
    }
    
    template<typename T>
    void deallocate(MemoryType type, Mad<T> p, size_t size)
    {
        free(p.raw(), size, type);
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
            return NoRawMad;
        }
        
        size_t offset = reinterpret_cast<char*>(addr) -_heapBase;
        if ((offset % _memoryInfo.blockSize) != 0) {
            return NoRawMad;
        }
        
        return static_cast<RawMad>(offset / _memoryInfo.blockSize);
    }
    
    const MemoryInfo& memoryInfo() const { return _memoryInfo; }

    static ROMString stringFromMemoryType(MemoryType);
    
protected:
    MemoryInfo _memoryInfo;

private:
    Mallocator() { init(); }

    using BlockId = RawMad;

    RawMad alloc(size_t size, MemoryType type);
    void free(RawMad, size_t size, MemoryType type);
    
    void coalesce(BlockId prev, BlockId next);

    uint16_t blockSizeFromByteSize(size_t size) { return (size + _memoryInfo.blockSize - 1) / _memoryInfo.blockSize; }
    
    void checkConsistency();
    
    static constexpr BlockId NoBlockId = static_cast<BlockId>(-1);

    struct Header
    {
#ifdef MEMORY_HEADER
        static constexpr uint16_t MAGIC = 0xBEEF;
        enum class Type : uint16_t { Free, Allocated };
        uint16_t magic;
        Type type;
#endif
        BlockId nextBlock;
        uint16_t sizeInBlocks;
    };
    
    Header* asHeader(BlockId b)
    {
        return reinterpret_cast<Header*>(_heapBase + (b * _memoryInfo.blockSize));
    }
    
    char* _heapBase = nullptr;
    BlockId _firstFreeBlock = 0;
    
    static Mallocator _mallocator;
};

template<typename T>
Mad<T>::Mad(const T* addr)
{
    *this = Mad(Mallocator::shared()->blockIdFromAddr(const_cast<T*>(addr)));
}

template<typename T>
inline T* Mad<T>::get() const { return reinterpret_cast<T*>(Mallocator::shared()->get(raw())); }

template<typename T>
inline void Mad<T>::destroyHelper(MemoryType type, uint16_t size, bool destruct)
{
    if (!valid() || size == 0) {
        return;
    }
    
    if (this->valid()) {
        // Only call the destructor if this isn't an array of objects.
        // Arrays call their desctructors themselves
        if (destruct) {
            get()->~T();
        }
        Mallocator::shared()->deallocate(type, *this, size);
    }
}

template<>
inline void Mad<char>::destroy(uint16_t size)
{
    if (size == 0) {
        return;
    }
    
    Mallocator::shared()->deallocate(MemoryType::Character, *this, size);
}

template<>
inline void Mad<uint8_t>::destroy(uint16_t size)
{
    if (size == 0) {
        return;
    }
    
    Mallocator::shared()->deallocate(MemoryType::Character, *this, size);
}

template<>
inline void Mad<Object>::destroy(uint16_t size)
{
    Mallocator::shared()->deallocate(MemoryType::Object, *this, size);
}

template<>
inline void Mad<NativeObject>::destroy(uint16_t size)
{
    Mallocator::shared()->deallocate(MemoryType::Native, *this, size);
}

template<typename T>
inline Mad<T> Mad<T>::create(MemoryType type, uint16_t n)
{
    Mad<T> obj = Mallocator::shared()->allocate<T>(type, sizeof(T) * n);
    
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
    return Mallocator::shared()->allocate<char>(MemoryType::Character, n);
}

template<>
inline Mad<uint8_t> Mad<uint8_t>::create(uint16_t n)
{
    return Mallocator::shared()->allocate<uint8_t>(MemoryType::Character, n);
}

}
