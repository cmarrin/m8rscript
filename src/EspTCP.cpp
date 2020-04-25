/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "EspTCP.h"

#include "Containers.h"
#include "SystemInterface.h"
#include "Value.h"
#include <cstdio>
#include <cstring>

using namespace m8r;

static constexpr Duration PollRate = 100ms;

void EspTCP::init(uint16_t port, IPAddr ip, EventFunction func)
{
    TCP::init(port, ip, func);
    if (ip) {
        _wifiClient = std::make_unique<WiFiClient>();
        _wifiClient->connect(static_cast<uint32_t>(ip), port);
    } else {
        _wifiServer = std::make_unique<WiFiServer>(port);
        _wifiServer->begin();
    }

    m8r::system()->startTimer(PollRate, true, [this] {
        if (_wifiServer && _wifiServer->hasClient()) {
            for (int i = 0; ; ++i) {
                if (i >= MaxConnections) {
                    addEvent(TCP::Event::Error, -1, "Too many connections on port");
                    break;
                }
                if (!_clients[i]) {
                    _clients[i] = _wifiServer->available();
                    addEvent(TCP::Event::Connected, i, nullptr);
                    break;
                }
            }
        }
    });
}

EspTCP::~EspTCP()
{
    // TODO: Close everything up
}

int32_t EspTCP::sendData(int16_t connectionId, const char* data, uint16_t length)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return -1;
    }
    if (!length) {
        length = ::strlen(data);
    }

    if (!_clients[connectionId]) {
        return -1;
    }

    return _clients[connectionId].write(data, length);
}

void EspTCP::disconnect(int16_t connectionId)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return;
    }
    
    // TODO: Implement

    addEvent(TCP::Event::Disconnected, connectionId, nullptr);
}

void EspTCP::handleEvents()
{
    TCP::handleEvents();
}
