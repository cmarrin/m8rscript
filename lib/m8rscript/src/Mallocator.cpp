/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Mallocator.h"
#include <cstdio>

using namespace m8r;

Mallocator Mallocator::_mallocator;

RawMad Mallocator::alloc(uint32_t size, MemoryType type)
{
    assert(type != MemoryType::Unknown);
    
    RawMad allocatedBlock = reinterpret_cast<RawMad>(::malloc(size));
    
    assert(_memoryInfo.numAllocations < std::numeric_limits<uint16_t>::max());
    ++_memoryInfo.numAllocations;
    _memoryInfo.totalAllocatedBytes += size;

    if (allocatedBlock == NoRawMad) {
        return NoRawMad;
    }
    
    uint32_t index = static_cast<uint32_t>(type);
    
    _memoryInfo.allocationsByType[index].count++;
    _memoryInfo.allocationsByType[index].size += size;

    return allocatedBlock;
}

void Mallocator::free(RawMad ptr, MemoryType type)
{
    // FIXME: How do we get the size? Pass it in?
    uint32_t size = 0;
    
    assert(type != MemoryType::Unknown);

    ::free(reinterpret_cast<void*>(ptr));

    assert(_memoryInfo.numAllocations > 0);
    --_memoryInfo.numAllocations;
    _memoryInfo.totalAllocatedBytes -= size;

    uint32_t index = static_cast<uint32_t>(type);
    assert(_memoryInfo.allocationsByType[index].count > 0);
    _memoryInfo.allocationsByType[index].count--;
    
    assert(_memoryInfo.allocationsByType[index].size >= size);
    _memoryInfo.allocationsByType[index].size -= size;
}

ROMString Mallocator::stringFromMemoryType(MemoryType type)
{
    switch(type) {
        case MemoryType::String:        return ROMSTR("String");
        case MemoryType::Character:     return ROMSTR("Char");
        case MemoryType::Object:        return ROMSTR("Object");
        case MemoryType::ExecutionUnit: return ROMSTR("ExecutionUnit");
        case MemoryType::Native:        return ROMSTR("Native");
        case MemoryType::Vector:        return ROMSTR("Vector");
        case MemoryType::UpValue:       return ROMSTR("UpValue");
        case MemoryType::Network:       return ROMSTR("Network");
        case MemoryType::Fixed:         return ROMSTR("Fixed");
        case MemoryType::NumTypes:
        case MemoryType::Unknown:
        default:                        return ROMSTR("Unknown");
    }
}
