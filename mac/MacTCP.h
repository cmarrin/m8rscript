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

class MacTCP : public TCP {
public:
    static constexpr int BufferSize = 1024;
    
    virtual ~MacTCP();
    
    virtual void disconnect(int16_t connectionId) override;
    
    void init(uint16_t port, IPAddr ip, EventFunction func);

private:
    virtual int32_t sendData(int16_t connectionId, const char* data, uint16_t length = 0) override;

    int _socketFD = -1;
    char _receiveBuffer[BufferSize];
    int _clientSockets[MaxConnections];
    Mutex _mutex; // protection for _clientSockets
    Thread _thread;
};

}
