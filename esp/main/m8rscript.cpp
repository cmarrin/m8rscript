
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "Application.h"
#include "Defines.h"
#include "Mallocator.h"
#include "MStream.h"
#include "SystemInterface.h"
#include "SystemTime.h"

#include <unistd.h>
#include <chrono>

static m8r::Duration MainTaskSleepDuration = 10ms;

extern "C" void app_main()
{
    m8r::Application* application = new m8r::Application(23);
    application->runAutostartTask();

    while(1) {
        application->runOneIteration();
        vTaskDelay(pdMS_TO_TICKS(MainTaskSleepDuration.ms()));
    }
}
