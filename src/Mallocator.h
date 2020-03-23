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

class NativeObject;
class Object;

#ifndef NDEBUG
#define MEMORY_PTR
#endif

#ifndef NDEBUG
#define CHECK_CONSISTENCY
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
#define DEBUG_MEMORY_HEADER
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
        T* t = nullptr;
        const U* u = t;
        (void) u;

        return Mad<U>(raw());
    }
    
    void reset() { *this = Mad<T>(); }
    
    template<typename X>
    struct assert_false : std::false_type { };
    void destroy() { destroy(T::memoryType()); }
    
    void destroyVector() { destroyHelper(MemoryType::Vector, false); }
    void destroy(MemoryType type) { destroyHelper(type, true); }

    static Mad<T> create(MemoryType, uint16_t n = 1);
    
    static Mad<T> create(uint16_t n) { static_assert(assert_false<T>::value, "Must specialize this function"); return Mad<T>(); }
    static Mad<T> create() { return create(T::memoryType(), 1); }

private:
    RawMad _raw = NoRawMad;
    
    void destroyHelper(MemoryType, bool destruct);
    
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
    Mad<T> allocate(MemoryType type, uint16_t nElements)
    {
        return Mad<T>(alloc(static_cast<uint32_t>(nElements) * sizeof(T), type, typeName<T>()));
    }
    
    template<typename T>
    void deallocate(MemoryType type, Mad<T> p)
    {
        free(p.raw(), type);
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
    
    const MemoryInfo& memoryInfo() const { showAllocationRecord(); return _memoryInfo; }

    static ROMString stringFromMemoryType(MemoryType);
    
    void checkConsistency() { checkConsistencyHelper(); }

protected:
    MemoryInfo _memoryInfo;

private:
    using BlockId = RawMad;

    RawMad alloc(uint32_t size, MemoryType type, const char* valueType);
    void free(RawMad, MemoryType type);
    
    void coalesce(BlockId prev, BlockId next);

    uint16_t blockSizeFromByteSize(size_t size) { return (size + _memoryInfo.blockSize - 1) / _memoryInfo.blockSize; }
    
#ifdef CHECK_CONSISTENCY
    void checkConsistencyHelper();
#else
    void checkConsistencyHelper() { }
#endif

    static constexpr BlockId NoBlockId = static_cast<BlockId>(-1);

    // We only want to showAllocationRecord on Mac
#ifdef DEBUG_MEMORY_HEADER
    void showAllocationRecord() const
#ifdef __APPLE__
    ;
#else
    { }
#endif
#else
    void showAllocationRecord() const { }
#endif
    
    struct Header
    {
        enum class Type : uint16_t { Free, Allocated };

#ifdef DEBUG_MEMORY_HEADER
        static constexpr uint16_t FREEMAGIC = 0xDEAD;
        static constexpr uint16_t ALLOCMAGIC = 0xBEEF;
        
        uint16_t magic;
        uint16_t _type : 1; // Type: Free or Allocated
        MemoryType memoryType;
        const char* name;
        
        Type type() const { return static_cast<Type>(_type); }
        void setType(Type t) { _type = static_cast<uint16_t>(t); }
#endif
        BlockId nextBlock;
        uint16_t sizeInBlocks;
    };

#ifdef DEBUG_MEMORY_HEADER
    void showMemoryHeaderError(BlockId, Header::Type type, int32_t blocksToFree) const;

    void checkMemoryHeader(BlockId block, Header::Type type, int32_t blocksToFree = -1) const
    {
        const Header* header = asHeader(block);
        
        if (type == Header::Type::Allocated) {
            if (header->magic != Header::ALLOCMAGIC || header->type() != Header::Type::Allocated) {
                showMemoryHeaderError(block, type, blocksToFree);
                return;
            }
            if (blocksToFree >= 0 && header->sizeInBlocks != blocksToFree) {
                showMemoryHeaderError(block, type, blocksToFree);
                return;
            }
        } else {
            if (header->magic != Header::FREEMAGIC || header->type() != Header::Type::Free) {
                showMemoryHeaderError(block, type, blocksToFree);
                return;
            }
        }
    }
#else
    void checkMemoryHeader(BlockId, Header::Type type, int32_t blocksToFree = -1) const { }
#endif

    Header* asHeader(BlockId b) { return reinterpret_cast<Header*>(_heapBase + (b * _memoryInfo.blockSize)); }
    const Header* asHeader(BlockId b) const { return reinterpret_cast<const Header*>(_heapBase + (b * _memoryInfo.blockSize)); }

    char* _heapBase = nullptr;
    BlockId _firstFreeBlock = 0;

#ifdef DEBUG_MEMORY_HEADER
    BlockId _firstAllocatedBlock = 0;
#endif

    static Mallocator _mallocator;
    Mutex _mutex;

};

template<typename T>
Mad<T>::Mad(const T* addr)
{
    *this = Mad(Mallocator::shared()->blockIdFromAddr(const_cast<T*>(addr)));
}

template<typename T>
inline T* Mad<T>::get() const { return reinterpret_cast<T*>(Mallocator::shared()->get(raw())); }

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

//template<>
//inline void Mad<Object>::destroy()
//{
//    if (!valid()) {
//        return;
//    }
//    Mallocator::shared()->deallocate(MemoryType::Object, *this);
//}
//
//template<>
//inline void Mad<NativeObject>::destroy()
//{
//    if (!valid()) {
//        return;
//    }
//    Mallocator::shared()->deallocate(MemoryType::Native, *this);
//}

}
