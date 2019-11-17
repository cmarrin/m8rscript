/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "UDP.h"

#include <dispatch/dispatch.h>

namespace m8r {

class MacUDP : public UDP {
public:
    static constexpr int BufferSize = 1024;

    void init(UDPDelegate*, uint16_t);
    virtual ~MacUDP();
    
    static void joinMulticastGroup(IPAddr);
    static void leaveMulticastGroup(IPAddr);
    
    virtual void send(IPAddr, uint16_t port, const char* data, uint16_t length = 0) override;
    virtual void disconnect() override;
    
private:    
    int _socketFD = -1;
    dispatch_queue_t _queue;
    char _receiveBuffer[BufferSize];
};

}
