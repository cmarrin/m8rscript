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
    
    uint16_t blocksToAlloc = blockSizeFromByteSize(size);
    
    BlockId freeBlock = _firstFreeBlock;
    BlockId prevBlock = NoBlockId;
    while (freeBlock != NoBlockId) {
        FreeHeader* header = block(freeBlock);
        assert(header->nextBlock != freeBlock);
        
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

void Mallocator::coalesce(BlockId prev, BlockId next)
{
    FreeHeader* header = block(prev);
    assert(prev + header->sizeInBlocks <= next);
    if (prev + header->sizeInBlocks == next) {
        // Coalesce
        assert(header->nextBlock == next);
        header->nextBlock = block(next)->nextBlock;
        header->sizeInBlocks += block(next)->sizeInBlocks;
    }
}

void Mallocator::free(RawMad p, size_t size)
{
    if (!_heapBase) {
        return;
    }
    
    BlockId newBlock = p;
    FreeHeader* newBlockHeader = block(newBlock);
    newBlockHeader->nextBlock = NoBlockId;
    newBlockHeader->sizeInBlocks = blockSizeFromByteSize(size);
    
    // Insert in free list
    BlockId freeBlock = _firstFreeBlock;
    BlockId prevBlock = NoBlockId;
    while (freeBlock != NoBlockId) {
        assert(newBlock != freeBlock);
        if (newBlock < freeBlock) {
            if (prevBlock == NoBlockId) {
                // Insert at the head
                newBlockHeader->nextBlock = _firstFreeBlock;
                _firstFreeBlock = newBlock;
            } else {
                newBlockHeader->nextBlock = freeBlock;
                block(prevBlock)->nextBlock = newBlock;
            }
            break;
        }
        prevBlock = freeBlock;
        freeBlock = block(freeBlock)->nextBlock;
    }
    
    if (freeBlock == NoBlockId) {
        // newBlock is beyond end of list. Add it to the tail
        block(prevBlock)->nextBlock = newBlock;
    }
    
    // If this is the only block (super unlikely), no coalescing is needed
    if (block(_firstFreeBlock)->nextBlock == NoBlockId) {
        return;
    }
    
    // 3 cases:
    //
    //      1) prevBlock is NoBlockId:
    //          newBlock is linked into free list at the head
    //          freeBlock is the block after newBlock
    //
    //      2) freeBlock is NoBlockId:
    //          newBlock is linked to the end of the free list
    //          prevBlock is the block before newBlock
    //
    //      3) newBlock is linked to the middle of the free list:
    //          prevBlock is the block before
    //          freeBlock is the block after
    
    if (prevBlock == NoBlockId) {
        coalesce(newBlock, freeBlock);
    } else if (freeBlock == NoBlockId) {
        coalesce(prevBlock, newBlock);
    } else {
        coalesce(newBlock, freeBlock);
        coalesce(prevBlock, newBlock);
    }
}

template<>
Mad<String> Mad<String>::create(const char* s, int32_t length)
{
    Mad<String> obj = Mallocator::shared()->allocate<String>(MemoryType::String, sizeof(String));
    new(obj.get()) String(s, length);
    return obj;
}

template<>
Mad<String> Mad<String>::create(const String& s)
{
    return create(s.c_str());
}

template<>
Mad<String> Mad<String>::create(String&& s)
{
    Mad<String> str = Mad<String>::create();
    *(str.get()) = s;
    return str;
}
