/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "esp_wifi.h"
#include "freertos/event_groups.h"

namespace m8r {

class RtosWifi
{
public:
    RtosWifi();

private:
    static esp_err_t eventHandler(void* ctx, system_event_t*);

    bool _inited = false;
    EventGroupHandle_t _eventGroup;
};

}
