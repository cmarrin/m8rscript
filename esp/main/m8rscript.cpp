
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
#include "esp_heap_caps.h"

#include "Application.h"
#include "Defines.h"
#include "Mallocator.h"
#include "SystemInterface.h"

#include <unistd.h>
#include <chrono>

void m8r::heapInfo(void*& start, uint32_t& size)
{
    static void* heapAddr = nullptr;
    static uint32_t heapSize = 0;
    
    if (!heapAddr) {
        heapSize = heap_caps_get_free_size(MALLOC_CAP_8BIT) - 10000;
        heapAddr = heap_caps_malloc(heapSize, MALLOC_CAP_8BIT);
    }
    
    start = heapAddr;
    size = heapSize;
}

extern "C" void app_main()
{
    printf("\n*** m8rscript v%d.%d - %s\n\n", m8r::MajorVersion, m8r::MinorVersion, __TIMESTAMP__);

    m8r::ROMString s = ROMSTR("*** HELLO ***\n");
    printf(s.value());
    
    m8r::Mad<char> chars = m8r::Mad<char>::create(22);
    strcpy(chars.get(), "It's Mad I tell you!\n");
    printf(chars.get());
    
    char* newstr = new char[22];
    strcpy(newstr, "Simply Mad!\n");
    printf(newstr);
    
    const m8r::MemoryInfo& info = m8r::Mallocator::shared()->memoryInfo();

    m8r::system()->printf(ROMSTR("Total heap: %d, free heap: %d\n"), info.heapSizeInBlocks * info.blockSize, info.freeSizeInBlocks * info.blockSize);
    
    m8r::Application application(23);
    printf("after application ctor\n");
    m8r::Application::mountFileSystem();
    printf("after application mountFileSystem\n");
    application.runLoop();
    printf("after application runLoop\n");
}
