
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
#include "MStream.h"
#include "SystemInterface.h"

#include <unistd.h>
#include <chrono>

extern "C" void app_main()
{
    printf("\n*** m8rscript v%d.%d - %s\n\n", m8r::MajorVersion, m8r::MinorVersion, __TIMESTAMP__);

    m8r::Application application(23);
    application.mountFileSystem();
    
    // Test filesystem
    m8r::String toPath("/foo");
    m8r::Mad<m8r::File> toFile(m8r::system()->fileSystem()->open(toPath.c_str(), m8r::FS::FileOpenMode::Write));
    if (!toFile->valid()) {
        printf("Error: unable to open '%s' for write - ", toPath.c_str());
        m8r::Error::showError(toFile->error());
        printf("\n");
    } else {
        toFile->write("Hello World", 11);
        if (!toFile->valid()) {
            printf("Error writing '%s' - ", toPath.c_str());
            m8r::Error::showError(toFile->error());
            printf("\n");
        } else {
            printf("Successfully wrote '%s'\n", toPath.c_str());
        }
        toFile->close();
    }

    toFile = m8r::Mad<m8r::File>(m8r::system()->fileSystem()->open(toPath.c_str(), m8r::FS::FileOpenMode::Read));
    if (!toFile->valid()) {
        printf("Error: unable to open '%s' for read - ", toPath.c_str());
        m8r::Error::showError(toFile->error());
        printf("\n");
    } else {
        char buf[12];
        int32_t result = toFile->read(buf, 11);
        if (!toFile->valid()) {
            printf("Error reading '%s' - ", toPath.c_str());
            m8r::Error::showError(toFile->error());
            printf("\n");
        } else if (result != 11) {
            printf("Wrong number of bytes read from '%s', expected 11, got %d\n", toPath.c_str(), result);
        } else {
            buf[11] = '\0';
            printf("Successfully read '%s' - '%s'\n", toPath.c_str(), buf);
        }
        toFile->close();
    }

    const m8r::MemoryInfo& info = m8r::Mallocator::shared()->memoryInfo();
    m8r::system()->printf(ROMSTR("Total heap: %d, free heap: %d\n"), info.heapSizeInBlocks * info.blockSize, info.freeSizeInBlocks * info.blockSize);
    application.runLoop();
}
