/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "TCP.h"

#include <ESP8266WiFi.h>

namespace m8r {

class EspTCP : public TCP {
public:
    static constexpr int BufferSize = 1024;
    
    virtual ~EspTCP();
    
    virtual void disconnect(int16_t connectionId) override;
    
    void init(uint16_t port, IPAddr ip, EventFunction func);

    virtual void handleEvents() override;

    virtual IPAddr clientIPAddr(int16_t connectionId) const override;

private:
    virtual int32_t sendData(int16_t connectionId, const char* data, uint16_t length = 0) override;
    
    void handleEvent(Event, int16_t connectionId, const char* data, uint16_t length);

    char _receiveBuffer[BufferSize];
    WiFiClient _clients[MaxConnections];

    std::unique_ptr<WiFiServer> _wifiServer;
    std::unique_ptr<WiFiClient> _wifiClient;
};

}
