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

#include <dispatch/dispatch.h>
#include <mutex>

namespace m8r {

class MacTCP : public TCP {
public:
    static constexpr int BufferSize = 1024;
    
    virtual ~MacTCP();
    
    virtual void send(int16_t connectionId, char c) override { send(connectionId, &c, 1); }
    virtual void send(int16_t connectionId, const char* data, uint16_t length = 0) override;
    virtual void disconnect(int16_t connectionId) override;
    
    void init(uint16_t port, IPAddr ip, EventFunction func);

private:
    int _socketFD = -1;
    dispatch_queue_t _queue;
    dispatch_semaphore_t _dispatchSemaphore;
    char _receiveBuffer[BufferSize];
    int _clientSockets[MaxConnections];
    std::mutex _mutex; // protection for _clientSockets
    bool _server;
};

}
