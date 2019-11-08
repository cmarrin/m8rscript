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

class Mallocator
{
public:
    template <class T, MemoryType Type = MemoryType::Unknown>
    class Alloc {
    public:
        typedef T value_type;

        T* allocate(std::size_t n)
        {
            n *= sizeof(T);
            return Mallocator::shared()->allocate<T>(Type, static_cast<uint32_t>(n));
        }

        void deallocate(T* p, std::size_t n)
        {
            n *= sizeof(T);
            Mallocator::shared()->deallocate<T>(Type, p, static_cast<uint32_t>(n));
        }

        template <typename U>
        struct rebind {
            typedef Alloc<U> other;
        };
    };

    template<typename T>
    T* allocate(MemoryType type, size_t size)
    {
        _list[static_cast<uint32_t>(type)].count++;
        _list[static_cast<uint32_t>(type)].size += size;

        assert(static_cast<uint32_t>(size) <= 0xffff);
        return reinterpret_cast<T*>(alloc(size));
    }
    
    template<typename T>
    void deallocate(MemoryType type, T* p, size_t size)
    {
        _list[static_cast<uint32_t>(type)].count--;
        _list[static_cast<uint32_t>(type)].size -= size;

        free(p, size);
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

protected:
    Entry _list[static_cast<uint32_t>(MemoryType::NumTypes)];

private:
    Mallocator();
    
    void* alloc(size_t size);
    void free(void*, size_t size);

    using BlockId = uint16_t;
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
