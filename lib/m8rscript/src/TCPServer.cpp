/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "TCPServer.h"

#include <unistd.h>

#ifdef MONITOR_TRAFFIC
#include <string>
#endif

using namespace m8r;

TCPServer::TCPServer(uint16_t port, CreateTaskFunction createTaskFunction, TCP::EventFunction eventFunction)
    : _createTaskFunction(createTaskFunction)
    , _eventFunction(eventFunction)
{
    Mad<TCP> socket = system()->createTCP(port, [this](TCP* tcp, TCP::Event event, int16_t connectionId, const char* data, int16_t length)
    {
        switch(event) {
            case TCP::Event::Connected:
                _connections[connectionId].task = _createTaskFunction();
                
                // Set the print function to send the printed string out the TCP channel
                _connections[connectionId].task->setConsolePrintFunction([&](const String& s) {
                    // Break it up into lines. We need to insert '\r'
                    Vector<String> v = s.split("\n");
                    for (uint32_t i = 0; i < v.size(); ++i) {
                        if (!v[i].empty()) {
                            _socket->send(connectionId, v[i].c_str(), v[i].size());
                        }
                        if (i == v.size() - 1) {
                            break;
                        }
                        _socket->send(connectionId, "\r\n", 2);
                    }
                });
                
                if (_connections[connectionId].task->error().code() != Error::Code::OK) {
                    _connections[connectionId].task->print(Error::formatError(_connections[connectionId].task->error().code()).c_str());
                    _connections[connectionId].task.reset();
                    _socket->disconnect(connectionId);
                } else {
                    // Give subclass a whack at the connection event before starting to run the task
                    // so it can do init, etc.
                    _eventFunction(tcp, event, connectionId, data, length);
                    
                    // Run the task
                    system()->taskManager()->run(_connections[connectionId].task, [connectionId, this](TaskBase*)
                    {
                        // On return from finished task, drop the connection
                        _socket->disconnect(connectionId);
                        _connections[connectionId].task.reset();
                    });
                }
                break;
            case TCP::Event::Disconnected:
                if (_connections[connectionId].task) {
                    system()->taskManager()->terminate(_connections[connectionId].task);
                    _connections[connectionId].task.reset();
                }
                break;
            case TCP::Event::ReceivedData:
                if (_connections[connectionId].task) {
                    _eventFunction(tcp, event, connectionId, data, length);
                }
                break;
            case TCP::Event::SentData:
                break;
            default:
                break;
        }
    });
    
    _socket = socket;
}

TCPServer::~TCPServer()
{
    _socket.destroy();
}
