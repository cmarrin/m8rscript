
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <Defines.h>
#include <Mallocator.h>

void m8r::heapInfo(void*& start, uint32_t& size)
{
    static void* heapAddr = nullptr;
    static uint32_t heapSize = 0;
    
    if (!heapAddr) {
        heapSize = ESP.getFreeHeap() - 10000;
        heapAddr = malloc(heapSize);
    }
    
    Serial.println("*** getting heapInfo");
    start = heapAddr;
    size = heapSize;
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n ***** Hello World! Free Heap=%d\n\n", ESP.getFreeHeap());
    
    m8r::ROMString s = ROMSTR("*** HELLO ***");
    Serial.println(s.value());
    
    m8r::Mad<char> chars = m8r::Mad<char>::create(22);
    strcpy(chars.get(), "It's Mad I tell you!");
    Serial.println(chars.get());
    
    char* newstr = new char[22];
    strcpy(newstr, "Simply Mad!");
    Serial.println(newstr);
}

void loop()
{
}
