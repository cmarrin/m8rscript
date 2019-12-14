
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "freertos/FreeRTOS.h"
#include "esp_system.h"

#include <Defines.h>
#include <Mallocator.h>

void m8r::heapInfo(void*& start, uint32_t& size)
{
    static void* heapAddr = nullptr;
    static uint32_t heapSize = 0;
    
    if (!heapAddr) {
        heapSize = heap_caps_get_free_size(MALLOC_CAP_32BIT) - 10000;
        heapAddr = malloc(heapSize);
    }
    
    printf("*** getting heapInfo: size=%d, addr=%p\n", heapSize, heapAddr);
    start = heapAddr;
    size = heapSize;
}

extern "C" void app_main()
{
    printf("Hello world!\n");
    printf("\n*** m8rscript v%d.%d - %s\n\n", m8r::MajorVersion, m8r::MinorVersion, m8r::BuildTimeStamp);
}
