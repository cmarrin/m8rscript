
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

#include "esp_spiffs.h"
#include <errno.h>
#include <fcntl.h>

esp_vfs_spiffs_conf_t conf = {
  .base_path = "/spiffs",
  .partition_label = NULL,
  .max_files = 5,
  .format_if_mount_failed = true
};

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
    sleep(2);
    printf("******** registering SPIFFS\n");
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            printf("Failed to mount or format filesystem\n");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            printf("Failed to find SPIFFS partition\n");
        } else {
            printf("Failed to initialize SPIFFS (%s)\n", esp_err_to_name(ret));
        }
        return;
    }
    printf("******** registration succeeded\n");

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        printf("Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
    } else {
        printf("Partition size: total: %d, used: %d\n", total, used);
    }
    
    printf("******** creating /spiffs/hello.txt\n");
    int fd = open("/spiffs/hello.txt", O_WRONLY | O_CREAT | O_EXCL);
    if (fd >= 0) {
        printf("******** Opened file for write: %d\n", fd);
        
        ssize_t result = write(fd, "Hello World", 11);
        printf("******** Wrote to file: result=%d\n", static_cast<int>(result));
        close(fd);

        // Check if destination file exists before renaming
        printf("******** checking for existance of /spiffs/foo.txt\n");
        struct stat st;
        if (stat("/spiffs/foo.txt", &st) == 0) {
            // Delete it if it exists
            printf("******** file exists deleting\n");
            unlink("/spiffs/foo.txt");
        }

        // Rename original file
        printf("******** Renaming file\n");
        if (rename("/spiffs/hello.txt", "/spiffs/foo.txt") != 0) {
            printf("******** Rename failed\n");
            return;
        }

        printf("******** opening /spiffs/foo.txt for read\n");
        int fd = open("/spiffs/foo.txt", O_RDONLY);
        if (fd >= 0) {
            printf("******** Opened file for read: %d\n", fd);
            
            char buf[12];
            ssize_t result = read(fd, buf, 11);
            buf[11] = '\0';
            printf("******** Read to file: result=%d, '%s'\n", static_cast<int>(result), buf);
            close(fd);
        }
    } else {
        printf("******* File open failed:%d\n", errno);
    }






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
    //m8r::Application::mountFileSystem();
    //printf("after application mountFileSystem\n");
    //application.runLoop();
    //printf("after application runLoop\n");
}
