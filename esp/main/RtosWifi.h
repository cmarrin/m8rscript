/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
#include "MString.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

namespace m8r {

class RtosWifi
{
public:
    RtosWifi() { }
    void start();

private:
    enum class State { InitialTry, Scan, ConnectAP, Retry, Failed };
    
    static esp_err_t eventHandler(void* ctx, system_event_t*);
    
    bool waitForConnect();
    void connectToSTA(const char* ssid, const char* pwd);
    void scanForNetworks();
    
    // Setup a wifi connection by starting an AP and presenting
    // a webpage with SSID choices and the ability to enter a
    // password
    bool setupConnection(const char* apName);

    State _state = State::InitialTry;
    EventGroupHandle_t _eventGroup;
    
    Vector<String> _ssidList;
};

}
