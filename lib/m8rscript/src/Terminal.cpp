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
    : _command(command)
{
    Mad<TCP> socket = system()->createTCP(port, [this](TCP*, TCP::Event event, int16_t connectionId, const char* data, int16_t length)
    {
        switch(event) {
            case TCP::Event::Connected:
                _shells[connectionId].task = Mad<Task>::create();
                
                // Set the print function to send the printed string out the TCP channel
                _shells[connectionId].task->setConsolePrintFunction([&](const String& s) {
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
                
                _shells[connectionId].task->init(_command.c_str());
                if (_shells[connectionId].task->error().code() != Error::Code::OK) {
                    _shells[connectionId].task->eu()->print(Error::formatError(_shells[connectionId].task->error().code()).c_str());
                    _shells[connectionId].task = Mad<Task>();
                    _socket->disconnect(connectionId);
                } else {
                    _socket->send(connectionId, _shells[connectionId].telnet.init().c_str());
                    
                    // Run the task
                    _shells[connectionId].task->run([connectionId, this](TaskBase*)
                    {
                        // On return from finished task, drop the connection
                        _socket->disconnect(connectionId);
                        _shells[connectionId].task.destroy();
                        _shells[connectionId].task = Mad<Task>();
                    });
                }
                break;
            case TCP::Event::Disconnected:
                if (_shells[connectionId].task.valid()) {
                    _shells[connectionId].task->terminate();
                    _shells[connectionId].task.destroy();
                    _shells[connectionId].task = Mad<Task>();
                }
                break;
            case TCP::Event::ReceivedData:
                if (_shells[connectionId].task.valid()) {
                    // Receiving characters. Pass them through Telnet
                    String toChannel, toClient;
                    for (int16_t i = 0; i < length; ++i) {
                        if (!data[i]) {
                            break;
                        }
                        KeyAction action = _shells[connectionId].telnet.receive(data[i], toChannel, toClient);
                    
                        if (!toClient.empty() || action != KeyAction::None) {
                            _shells[connectionId].task->receivedData(toClient, action);
                        }
                        if (!toChannel.empty()) {
                            _socket->send(connectionId, toChannel.c_str(), toChannel.size());
                        }
                    }
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

Terminal::~Terminal()
{
    _socket.destroy();
}
