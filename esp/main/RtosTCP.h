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

namespace m8r {

class RtosTCP : public TCP {
public:
    static constexpr int BufferSize = 1024;
    
    virtual ~RtosTCP();
    
    virtual void disconnect(int16_t connectionId) override;
    
    void init(uint16_t port, IPAddr ip, EventFunction func);

    virtual void handleEvents() override;

    virtual IPAddr clientIPAddr(int16_t connectionId) const override;

private:
    static constexpr int InvalidFD = -1;
    virtual int32_t sendData(int16_t connectionId, const char* data, uint16_t length = 0) override;
    
    void handleEvent(Event, int16_t connectionId, const char* data, uint16_t length);
    
    static void staticServerTask(void* data) { reinterpret_cast<RtosTCP*>(data)->serverTask(); }
    static void staticClientTask(void* data) { reinterpret_cast<RtosTCP*>(data)->clientTask(); }
    
    void serverTask();
    void clientTask();

    int _socketFD = InvalidFD;
    char _receiveBuffer[BufferSize];
    int _clientSockets[MaxConnections];
};

}
