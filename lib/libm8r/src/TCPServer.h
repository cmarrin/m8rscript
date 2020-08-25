/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "SharedPtr.h"
#include "TCP.h"

namespace m8r {

// TCPServer
//
// This is a wrapper around the raw TCP class which spawns a task each time a connection is made.
// Communications to and from that connection go to that task.

class Task;

class TCPServer {
public:
    using CreateTaskFunction = std::function<SharedPtr<Task>()>;
    
    TCPServer(uint16_t port, CreateTaskFunction, TCP::EventFunction);
    ~TCPServer();
    
    void handleEvents()
    {
        if (_socket.valid()) {
            _socket->handleEvents();
        }
    }

protected:
    Mad<TCP> _socket;

    struct Connection
    {
        Connection() { }
        SharedPtr<Task> task;
    };
    
    Connection _connections[TCP::MaxConnections];
    
    CreateTaskFunction _createTaskFunction;
    TCP::EventFunction _eventFunction;
};
    
}
