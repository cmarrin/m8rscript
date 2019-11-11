/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Mallocator.h"

#include "SystemInterface.h"

using namespace m8r;

Mallocator Mallocator::_mallocator;

Mallocator::Mallocator()
{
    void* base;
    uint32_t size;
    system()->heapInfo(base, size);
    
    // Limit is one less block, to provide for a NoBlockId value
    if (size < (1024 * 256 - 4)) {
        _memoryInfo.blockSize = 4;
    } else if (size < (1024 * 512 - 8)) {
        _memoryInfo.blockSize = 8;
    } else if (size < (1024 * 1024 - 16)) {
        _memoryInfo.blockSize = 16;
    } else {
        system()->printf(ROMSTR("Exceeded maximum heap size of 1MB\n"));
        return;
    }
    
    _heapBase = reinterpret_cast<char*>(base);
    _memoryInfo.heapSizeInBlocks = static_cast<uint16_t>(size / _memoryInfo.blockSize);
    
    block(0)->nextBlock = NoBlockId;
    block(0)->sizeInBlocks = _memoryInfo.heapSizeInBlocks;
    _firstFreeBlock = 0;
    _memoryInfo.freeSizeInBlocks = _memoryInfo.heapSizeInBlocks;
}

RawMad Mallocator::alloc(size_t size)
{
    if (!_heapBase) {
        return Id<RawMad>().raw();
    }
    
    uint16_t blocksToAlloc = (size + _memoryInfo.blockSize - 1) / _memoryInfo.blockSize;
    
    BlockId freeBlock = _firstFreeBlock;
    BlockId prevBlock = NoBlockId;
    while (freeBlock != NoBlockId) {
        FreeHeader* header = block(freeBlock);
        if (header->sizeInBlocks >= blocksToAlloc) {
            if (blocksToAlloc == header->sizeInBlocks) {
                // Take the whole thing
                if (prevBlock == NoBlockId) {
                    _firstFreeBlock = header->nextBlock;
                } else {
                    block(prevBlock)->nextBlock = header->nextBlock;
                }
                break;
            } else {
                // Take the tail end of the block and resize
                header->sizeInBlocks -= blocksToAlloc;
                freeBlock += header->sizeInBlocks;
                break;
            }
        }
        
        prevBlock = freeBlock;
        freeBlock = header->nextBlock;
    }
    
    if (freeBlock == NoBlockId) {
        return Id<RawMad>().raw();
    }
    
    assert(_memoryInfo.numAllocations < std::numeric_limits<uint16_t>::max());
    ++_memoryInfo.numAllocations;
    _memoryInfo.freeSizeInBlocks -= blocksToAlloc;
    return freeBlock;
}

void Mallocator::free(RawMad p, size_t size)
{
    if (!_heapBase) {
        return;
    }
    
    // TODO: Implement
}

//const MemoryInfo& Mallocator::memoryInfo() 
//{
//    info.heapSize = _heapBlockCount * _blockSize;
//    info.freeSize = _freeBlockCount;
//    info.numAllocations = _numAllocations;
//    
//    info.numAllocationsByType.clear();
//    Entry _list[static_cast<uint32_t>(MemoryType::NumTypes)];
//    for (int i = 0; i < static_cast<uint32_t>(MemoryType::NumTypes); ++i) {
//        ingo.numAllocationsByType.push_back(_list[i]);
//    }
//}

template<>
Mad<String> Mad<String>::create(const char* s, int32_t length)
{
    Mad<String> obj = Mallocator::shared()->allocate<String>(sizeof(String));
    new(obj.get()) String(s, length);
    return obj;
}
