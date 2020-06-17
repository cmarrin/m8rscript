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

// HTTPServer
//
// Build on top of TCP class
// Adapted from https://github.com/konteck/wpp

class HTTPServer {
public:
    class Request;
    
    HTTPServer(uint16_t port, const char* rootDir);
    ~HTTPServer() { }
    
    static String dateString();
    
    void sendResponseHeader(int16_t connectionId, uint32_t size);
    
    void handleEvents()
    {
        if (_socket.valid()) {
            _socket->handleEvents();
        }
    }

private:
    Mad<TCP> _socket;
};
    
}
