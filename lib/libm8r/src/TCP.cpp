/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "TCP.h"

#include "Defines.h"
#include "Mallocator.h"
#include "SystemInterface.h"

using namespace m8r;

void TCP::send(int16_t connectionId, const char* data, uint16_t length)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return;
    }
    if (!length) {
        length = ::strlen(data);
    }
    
    if (_server) {
        int32_t result = sendData(connectionId, data, length);
        if (result < 0) {
            _eventFunction(this, TCP::Event::Error, connectionId, "send (server) failed", -1);
        }
    } else {
        int32_t result = sendData(connectionId, data, length);
        if (result < 0) {
            _eventFunction(this, TCP::Event::Error, connectionId, "send (client) failed", -1);
        }
    }
    
    _eventFunction(this, TCP::Event::SentData, connectionId, data, length);
}

void TCP::handleEvents()
{
    for (auto it : _events) {
        _eventFunction(this, it.event, it.connectionId, it.data.size() ? &(it.data[0]) : nullptr, it.data.size());
    }
    _events.clear();
}
