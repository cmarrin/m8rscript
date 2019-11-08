/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "SystemInterface.h"

#include "TaskManager.h"

using namespace m8r;

void SystemInterface::lock()
{
    taskManager()->lock();
}

void SystemInterface::unlock()
{
    taskManager()->unlock();
}

void SystemInterface::runLoop()
{
    taskManager()->runLoop();
}

void SystemInterface::memoryInfo(MemoryInfo& info)
{
#ifdef USE_UMM
    info.freeSize = static_cast<uint32_t>(umm_free_heap_size());
    info.numAllocations = ummHeapInfo.usedEntries;
#else
    info.freeSize = 0;
    info.numAllocations = 0;
#endif
}
