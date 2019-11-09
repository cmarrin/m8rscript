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

Mallocator* Mallocator::_sharedMallocator = nullptr;

Mallocator::Mallocator()
{
    void* base;
    uint32_t size;
    system()->heapInfo(base, size);
    
    // Limit is one less block, to provide for a NoBlockId value
    if (size < (1024 * 256 - 4)) {
        _blockSize = 4;
    } else if (size < (1024 * 512 - 8)) {
        _blockSize = 8;
    } else if (size < (1024 * 1024 - 16)) {
        _blockSize = 16;
    } else {
        system()->printf(ROMSTR("Exceeded maximum heap size of 1MB\n"));
        return;
    }
    
    _heapBase = reinterpret_cast<char*>(base);
    _heapBlockCount = static_cast<uint16_t>(size / _blockSize);
    
    block(0)->nextBlock = NoBlockId;
    block(0)->sizeInBlocks = _heapBlockCount;
    _firstFreeBlock = 0;
}

RawMad Mallocator::alloc(size_t size)
{
    if (!_heapBase) {
        return Id<RawMad>().raw();
    }
    
    SizeInBlocks blockToAlloc = (size + _blockSize - 1) / _blockSize;
    
    BlockId freeBlock = _firstFreeBlock;
    BlockId prevBlock = NoBlockId;
    while (freeBlock != NoBlockId) {
        FreeHeader* header = block(freeBlock);
        if (header->sizeInBlocks >= blockToAlloc) {
            if (blockToAlloc == header->sizeInBlocks) {
                // Take the whole thing
                if (prevBlock == NoBlockId) {
                    _firstFreeBlock = header->nextBlock;
                } else {
                    block(prevBlock)->nextBlock = header->nextBlock;
                }
                return freeBlock;
            } else {
                // Take the tail end of the block and resize
                header->sizeInBlocks -= blockToAlloc;
                return freeBlock + header->sizeInBlocks;
            }
        }
        
        prevBlock = freeBlock;
        freeBlock = header->nextBlock;
    }
    
    return Id<RawMad>().raw();
}

void Mallocator::free(RawMad p, size_t size)
{
    if (!_heapBase) {
        return;
    }
    
    // TODO: Implement
}

template<>
Mad<String> Mad<String>::create(const char* s, int32_t length)
{
    Mad<String> obj = Mallocator::shared()->allocate<String>(sizeof(String));
    new(obj.get()) String(s, length);
    return obj;
}
