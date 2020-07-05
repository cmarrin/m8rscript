/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "MString.h"
#include <cstdint>

namespace m8r {

// HTTPServer
//
// Build on top of TCP class
// Adapted from https://github.com/konteck/wpp

class TCP;

class HTTPServer {
public:
    enum class Method { ANY, GET, PUT, POST, DELETE };
    using RequestFunction = std::function<void(const String& uri, Method)>;
    
    HTTPServer(uint16_t port, const char* rootDir, bool dirAccess = true);
    ~HTTPServer() { }

    void handleEvents();
    
    HTTPServer* on(const String& uri, RequestFunction);
    HTTPServer* on(const String& uri, Method, RequestFunction);
    HTTPServer* on(const String& uri, const String& path, bool dirAccess = true);

private:
    class Request;
    
    static String dateString();
        
    void sendResponseHeader(int16_t connectionId, uint32_t size);
    
    struct RequestHandler
    {
        RequestHandler() { }
        
        RequestHandler(const String& uri, Method method, RequestFunction f)
            : _uri(uri)
            , _method(method)
            , _requestHandler(f)
        { }
        
        RequestHandler(const String& uri, Method method, const String& path, bool dirAccess = true)
            : _uri(uri)
            , _method(method)
            , _path(path)
            , _dirAccess(dirAccess)
        { }

        String _uri;
        Method _method = Method::ANY;
        RequestFunction _requestHandler;
        String _path;
        bool _dirAccess = true;
    };

    Vector<RequestHandler> _requestHandlers;
    
    Mad<TCP> _socket;
};
    
}
