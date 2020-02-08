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

void* operator new(size_t size)
{
    printf("================================ new %d\n", (int) size);
    return ::malloc(size);
}

void operator delete(void* p) noexcept
{
    ::free(p);
}

Mallocator Mallocator::_mallocator;

void Mallocator::init()
{
    if (_heapBase) {
        return;
    }
    
    void* base;
    uint32_t size;
    heapInfo(base, size);
    if (base == nullptr) {
        return;
    }
    
    // Limit is one less block, to provide for a NoBlockId value
    if (size < (1024 * 256 - 4)) {
        _memoryInfo.blockSize = 4;
    } else if (size < (1024 * 512 - 8)) {
        _memoryInfo.blockSize = 8;
    } else if (size < (1024 * 1024 - 16)) {
        _memoryInfo.blockSize = 16;
    } else {
        // Exceeded maximum heap size of 1MB
        assert(0);
        return;
    }
    
    _heapBase = reinterpret_cast<char*>(base);
    _memoryInfo.heapSizeInBlocks = static_cast<uint16_t>(size / _memoryInfo.blockSize);
    
    asHeader(0)->nextBlock = NoBlockId;
    asHeader(0)->sizeInBlocks = _memoryInfo.heapSizeInBlocks;

#ifdef DEBUG_MEMORY_HEADER
    asHeader(0)->magic = Header::FREEMAGIC;
    asHeader(0)->setType(Header::Type::Free);
    asHeader(0)->memoryType = MemoryType::Unknown;
    _firstAllocatedBlock = NoBlockId;
#endif
    
    _firstFreeBlock = 0;
    _memoryInfo.freeSizeInBlocks = _memoryInfo.heapSizeInBlocks;
}

RawMad Mallocator::alloc(uint32_t size, MemoryType type, const char* valueType)
{
    Mallocator::shared()->init();

    if (!_heapBase) {
        return NoRawMad;
    }

    checkConsistency();
    assert(type != MemoryType::Unknown);
    assert(static_cast<uint32_t>(size) <= 0xffff);
    
    uint16_t blocksToAlloc = blockSizeFromByteSize(size);

    blocksToAlloc += blockSizeFromByteSize(sizeof(Header));
    
    BlockId freeBlock = _firstFreeBlock;
    BlockId prevBlock = NoBlockId;
    while (freeBlock != NoBlockId) {
        Header* header = asHeader(freeBlock);
        assert(header->nextBlock != freeBlock);

        checkMemoryHeader(freeBlock, Header::Type::Free);

        if (header->sizeInBlocks >= blocksToAlloc) {
            if (blocksToAlloc == header->sizeInBlocks) {
                // Take the whole thing
                if (prevBlock == NoBlockId) {
                    _firstFreeBlock = header->nextBlock;
                } else {
                    asHeader(prevBlock)->nextBlock = header->nextBlock;
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
        mallocatorUnlock();
        return NoRawMad;
    }
    
    RawMad allocatedBlock = freeBlock;
    
    assert(_memoryInfo.numAllocations < std::numeric_limits<uint16_t>::max());
    ++_memoryInfo.numAllocations;
    assert(_memoryInfo.freeSizeInBlocks >= blocksToAlloc);
    _memoryInfo.freeSizeInBlocks -= blocksToAlloc;

    if (allocatedBlock == NoBlockId) {
        mallocatorUnlock();
        return NoBlockId;
    }
    
    Header* header = asHeader(allocatedBlock);
    header->nextBlock = NoBlockId;
    header->sizeInBlocks = blocksToAlloc;
#ifdef DEBUG_MEMORY_HEADER
    header->magic = Header::ALLOCMAGIC;
    header->setType(Header::Type::Allocated);
    header->memoryType = type;
    header->name = valueType;
    header->nextBlock = _firstAllocatedBlock;
    _firstAllocatedBlock = allocatedBlock;
#endif
    allocatedBlock += blockSizeFromByteSize(sizeof(Header));

    uint32_t index = static_cast<uint32_t>(type);
    
    _memoryInfo.allocationsByType[index].count++;
    _memoryInfo.allocationsByType[index].sizeInBlocks += blocksToAlloc;
    
    checkConsistency();

    mallocatorUnlock();
    return allocatedBlock;
}

void Mallocator::coalesce(BlockId prev, BlockId next)
{
    Header* header = asHeader(prev);
    assert(prev + header->sizeInBlocks <= next);
    if (prev + header->sizeInBlocks == next) {
        // Coalesce
        assert(header->nextBlock == next);
        header->nextBlock = asHeader(next)->nextBlock;
        header->sizeInBlocks += asHeader(next)->sizeInBlocks;
    }
}

void Mallocator::free(RawMad ptr, MemoryType type)
{
    if (!_heapBase) {
        return;
    }
    
    mallocatorLock();
    checkConsistency();
    assert(type != MemoryType::Unknown);

    BlockId blockToFree = ptr;
    blockToFree -= blockSizeFromByteSize(sizeof(Header));
    
    uint32_t size = asHeader(blockToFree)->sizeInBlocks * _memoryInfo.blockSize;
    uint16_t blocksToFree = blockSizeFromByteSize(size);

    // Check to make sure this is a valid block
    checkMemoryHeader(blockToFree, Header::Type::Allocated, blocksToFree);

#ifdef DEBUG_MEMORY_HEADER
    asHeader(blockToFree)->magic = Header::FREEMAGIC;
    asHeader(blockToFree)->setType(Header::Type::Free);
    asHeader(blockToFree)->memoryType = MemoryType::Unknown;

    // Disconnect from allocated list
    BlockId prev = NoBlockId;
    
    for (BlockId block = _firstAllocatedBlock; block != NoBlockId; block = asHeader(block)->nextBlock) {
        if (block == blockToFree) {
            if (prev == NoBlockId) {
                _firstAllocatedBlock = asHeader(blockToFree)->nextBlock;
            } else {
                asHeader(prev)->nextBlock = asHeader(blockToFree)->nextBlock;
            }
            break;
        }
        prev = block;
    }
#endif
    
    BlockId newBlock = blockToFree;
    Header* newBlockHeader = asHeader(newBlock);
    newBlockHeader->nextBlock = NoBlockId;
    newBlockHeader->sizeInBlocks = blocksToFree;

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
                asHeader(prevBlock)->nextBlock = newBlock;
            }
            break;
        }
        prevBlock = freeBlock;
        freeBlock = asHeader(freeBlock)->nextBlock;
        assert(freeBlock > prevBlock || freeBlock == NoBlockId);
        if (freeBlock < prevBlock && freeBlock != NoBlockId) {
            // Internal Error: Freeblock loop detected
            abort();
        }
    }
    
    if (freeBlock == NoBlockId) {
        // newBlock is beyond end of list. Add it to the tail
        asHeader(prevBlock)->nextBlock = newBlock;
    }
    
    // If this is the only block (super unlikely), no coalescing is needed
    if (asHeader(_firstFreeBlock)->nextBlock != NoBlockId) {
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
            assert(asHeader(newBlock)->nextBlock == freeBlock);
            coalesce(newBlock, freeBlock);
        } else if (freeBlock == NoBlockId) {
            assert(asHeader(prevBlock)->nextBlock == newBlock);
            coalesce(prevBlock, newBlock);
        } else {
            assert(asHeader(newBlock)->nextBlock == freeBlock);
            assert(asHeader(prevBlock)->nextBlock == newBlock);
            coalesce(newBlock, freeBlock);
            coalesce(prevBlock, newBlock);
        }
    }

    assert(_memoryInfo.numAllocations > 0);
    --_memoryInfo.numAllocations;
    _memoryInfo.freeSizeInBlocks += blocksToFree;
    assert(_memoryInfo.freeSizeInBlocks <= _memoryInfo.heapSizeInBlocks);

    uint32_t index = static_cast<uint32_t>(type);
    assert(_memoryInfo.allocationsByType[index].count > 0);
    _memoryInfo.allocationsByType[index].count--;
    assert(_memoryInfo.allocationsByType[index].sizeInBlocks >= blocksToFree);
    _memoryInfo.allocationsByType[index].sizeInBlocks -= blocksToFree;
    checkConsistency();

    mallocatorUnlock();
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

#ifdef CHECK_CONSISTENCY
void Mallocator::checkConsistencyHelper()
{
    for (BlockId block = _firstFreeBlock; block != NoBlockId; block = asHeader(block)->nextBlock) {
        Header* header = asHeader(block);
        assert(header && (header->nextBlock > block || header->nextBlock == NoBlockId));
        assert(header && (header->nextBlock == NoBlockId || block + header->sizeInBlocks < header->nextBlock));

        // Check to make sure this is a valid block
        checkMemoryHeader(block, Header::Type::Free);
    }
    
#ifdef DEBUG_MEMORY_HEADER
    // Check allocated blocks
    for (BlockId block = _firstAllocatedBlock; block != NoBlockId; block = asHeader(block)->nextBlock) {
        // Check to make sure this is a valid block
        checkMemoryHeader(block, Header::Type::Allocated);
    }
#endif
}
#endif

#ifdef DEBUG_MEMORY_HEADER

void Mallocator::showMemoryHeaderError(BlockId block, Header::Type type, int32_t blocksToFree) const
{
    const Header* header = asHeader(block);

    if (type == Header::Type::Allocated) {
        ::printf("***** Allocated Memory Header error at block %d (%p), memory type %d, name '%s'\n", block, header, static_cast<uint32_t>(header->memoryType), header->name);
        if (header->magic != Header::ALLOCMAGIC) {
            ::printf("      expected ALLOCMAGIC, got 0x%08x\n", header->magic);
        }
        if (header->type() != Header::Type::Allocated) {
            ::printf("      expected Header::Type::Allocated, got 0x%04hx\n", static_cast<uint16_t>(header->type()));
        }
        if (blocksToFree >= 0 && header->sizeInBlocks != blocksToFree) {
            ::printf("      sizes don't match - allocation contains %d blocks, request to free %d blocks\n", header->sizeInBlocks, blocksToFree);
        }
    } else {
        ::printf("***** Free Memory Header error at addr block %d (%p)\n", block, header);
        if (header->magic != Header::FREEMAGIC) {
            ::printf("      expected FREEMAGIC, got 0x%08x\n", header->magic);
        }
        if (header->type() != Header::Type::Free) {
            ::printf("      expected Header::Type::Free, got 0x%04hx\n", static_cast<uint16_t>(header->type()));
            return;
        }
    }
    abort();
}

#ifdef __APPLE__
void Mallocator::showAllocationRecord() const
{
    for (BlockId block = _firstAllocatedBlock; block != NoBlockId; block = asHeader(block)->nextBlock) {
        const Header* header = asHeader(block);
        if (header->memoryType != MemoryType::Vector) {
            continue;
        }
        int32_t size = static_cast<int32_t>(header->sizeInBlocks) * _memoryInfo.blockSize;
        printf("%15s (%d) size=%8d bytes of type %s\n",
                         stringFromMemoryType(header->memoryType).value(),
                         static_cast<int32_t>(block),
                         static_cast<int32_t>(size),
                         header->name ?: "*** unknown ***");
    }

}
#endif
#endif
