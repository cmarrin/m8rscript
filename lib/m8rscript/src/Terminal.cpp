/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Terminal.h"

#include "ExecutionUnit.h"

#include <unistd.h>

#ifdef MONITOR_TRAFFIC
#include <string>
#endif

using namespace m8r;

Terminal::Terminal(uint16_t port, const char* command)
    : TCPServer(port,
    [this]()
    {
        Mad<Task> task = Mad<Task>::create(MemoryType::Native);
        task->init(_command.c_str());
        return task;
    },
    [this](TCP*, TCP::Event event, int16_t connectionId, const char* data, int16_t length)
    {
        switch(event) {
            default: break;
            case TCP::Event::Connected:
                _telnets[connectionId].reset();
                _socket->send(connectionId, _telnets[connectionId].makeInitString().c_str());
                break;
            case TCP::Event::ReceivedData:
                if (_connections[connectionId].task.valid()) {
                    // Receiving characters. Pass them through Telnet
                    String toChannel, toClient;
                    for (int16_t i = 0; i < length; ++i) {
                        if (!data[i]) {
                            break;
                        }
                        KeyAction action = _telnets[connectionId].receive(data[i], toChannel, toClient);
                    
                        if (!toClient.empty() || action != KeyAction::None) {
                            _connections[connectionId].task->receivedData(toClient, action);
                        }
                        if (!toChannel.empty()) {
                            _socket->send(connectionId, toChannel.c_str(), toChannel.size());
                        }
                    }
                }
                break;
        }
    })
    , _command(command)
{
}
