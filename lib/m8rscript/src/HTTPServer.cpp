/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "HTTPServer.h"

#include "Containers.h"
#include "MFS.h"
#include "MString.h"
#include "SystemInterface.h"
#include "SystemTime.h"
#include "TCP.h"


using namespace m8r;

static constexpr char _GET[] = "GET";
static constexpr char _PUT[] = "PUT";
static constexpr char _POST[] = "POST";
static constexpr char _DELETE[] = "DELETE";

struct Methods
{
    const char* str;
    HTTPServer::Method method;
};

Methods _methods[] = {
    { _GET, HTTPServer::Method::GET },
    { _PUT, HTTPServer::Method::PUT },
    { _POST, HTTPServer::Method::POST },
    { _DELETE, HTTPServer::Method::DELETE },
};

static HTTPServer::Method toMethod(const char* s)
{
    for (const auto& it : _methods) {
        if (::strcmp(it.str, s) == 0) {
            return it.method;
        }
    }
    return HTTPServer::Method::ANY;
}

static String toString(HTTPServer::Method method)
{
    for (const auto& it : _methods) {
        if (it.method == method) {
            return it.str;
        }
    }
    return String("*** Unknown ***");
}

// This parser doesn't handle the entire HTTP standard. In
// fact it puts extra constraints on the query string. Query
// values follow the '?' of the uri. Query values are 
// separated by '&' and each query value is a key/value pair
// with the form: <key>=<value> with no spaces allowed.
static void parseRequest(const String& s, HTTPServer::Request& request)
{
    // Split into lines
    Vector<String> lines = s.split("\n");
    
    // Handle method line
    Vector<String> line = lines[0].split(" ");
    if (line.size() != 3) {
        return;
    }
    
    request.method = toMethod(line[0].c_str());;
    if (request.method == HTTPServer::Method::ANY) {
        // Invalid
        return;
    }
    
    request.path = line[1];
    
    // Split out the params
    Vector<String> pathParams = request.path.split("?");
    if (pathParams.size() > 1) {
        request.path = pathParams[0];
        
        // Split the params
        Vector<String> params = pathParams[1].split("&");
        for (const auto& it : params) {
            Vector<String> parts = it.split("=");
            if (parts.size() != 2) {
                continue; // Not well formed
            }
            request.params.emplace(parts[0], parts[1]);
        }
    }
    
    // Parse the remaining lines
    for (int i = 1; i < lines.size(); ++i) {
        if (lines[i].empty()) {
            continue;
        }
        Vector<String> pair = lines[i].split(":");
        if (pair.size() != 2) {
            continue;
        }
        request.headers.emplace(pair[0].trim(), pair[1].trim());
    }
        
    request.valid = true;
}

HTTPServer::HTTPServer(uint16_t port, const char* rootDir, bool dirAccess)
    : _rootDir(rootDir)
    , _dirAccess(dirAccess)
{
    Mad<TCP> socket = system()->createTCP(port, [this](TCP* tcp, TCP::Event event, int16_t connectionId, const char* data, int16_t length)
    {
        if ((connectionId < 0 || connectionId >= TCP::MaxConnections) && event != TCP::Event::Error) {
            system()->printf("******** HTTPServer Internal Error: Invalid connectionId = %d\n", connectionId);
            _socket->disconnect(connectionId);
            return;
        }

        switch(event) {
            case TCP::Event::Connected:
                system()->printf("HTTPServer: new connection, connectionId=%d, ip=%s, port=%d\n", 
                                 connectionId, tcp->clientIPAddr(connectionId).toString().c_str(), tcp->port());
                break;
            case TCP::Event::Disconnected:
                system()->printf("HTTPServer: disconnecting, connectionId=%d, ip=%s, port=%d\n", 
                                 connectionId, tcp->clientIPAddr(connectionId).toString().c_str(), tcp->port());
                break;
            case TCP::Event::ReceivedData: {
                // FIXME: Handle incoming data
                // This might be a request (GET, PUT, POST) or upoaded data for a PUT
                String header(data, length);
                printf("******** Received HTTP data:\n%s", header.c_str());
                
                Request req;
                parseRequest(header, req);
                if (!req.valid) {
                    system()->printf("******** HTTPServer Invalid header\n");
                    break;
                }
                
                bool handled = false;
                
                // Find exact path matches first
                for (const auto& it : _requestHandlers) {
                    if (it._uri == req.path) {
                        if (it._requestHandler) {
                            it._requestHandler(it._uri, "", req, connectionId);
                        }
                        handled = true;
                        break;
                    }
                }
                
                if (!handled) {
                    // Now look for prefix matches
                    const RequestHandler* foundRequest = nullptr;
                    
                    for (const auto& it : _requestHandlers) {
                        if (it._uri.size() <= req.path.size()) {
                            continue;
                        }
                        
                        if (it._uri == String(req.path.c_str(), it._uri.size())) {
                            if (!foundRequest || foundRequest->_uri.size() < it._uri.size()) {
                                foundRequest = &it;
                            }
                        }
                    }
                    if (foundRequest) {
                        String suffix = String(req.path.c_str() + foundRequest->_uri.size());
                        if (foundRequest->_requestHandler) {
                            foundRequest->_requestHandler(foundRequest->_uri, suffix, req, connectionId);
                        }
                        handled = true;
                    }
                }

                if (!handled) {
                    system()->printf("******** HTTPServer Error: no %s method handler for uri '%s'\n",
                                            toString(req.method).c_str(), req.path.c_str());
                }
                break;
            }
            case TCP::Event::SentData:
                break;
            case TCP::Event::Error:
                system()->printf("******** HTTPServer Error: code=%d (%s)\n", connectionId, data);
            default:
                break;
        }
    });
    
    _socket = socket;
}

void HTTPServer::sendResponseHeader(int16_t connectionId, uint32_t size)
{
    // This is for a valid response
    String s = String::format("HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n",
                                 dateString().c_str(), size);
    _socket->send(connectionId, s.c_str());
}

String HTTPServer::dateString()
{
    // Format is <abbrev wkday>, <day> <abbrev month> <year> <hr>:<min>:<sec> <tz name>
    Time now = Time::now();
    Time::Elements el;
    now.elements(el);
    
    // FIXME: Get timezone
    return String::format("%s, %d %s %d %02d:%02d:%02d *", 
                             el.dayString().c_str(), el.day, el.monthString().c_str(), el.year, el.hour, el.minute, el.second);
}

void HTTPServer::handleEvents()
{
    if (_socket.valid()) {
        _socket->handleEvents();
    }
}

void HTTPServer::on(const String& uri, RequestFunction f)
{
    _requestHandlers.emplace_back(uri, f);
}

void HTTPServer::on(const String& uri, const String& path, bool dirAccess)
{
    _requestHandlers.emplace_back(uri, 
    [this, path, dirAccess](const String& uri, const String&, const Request& request, int16_t connectionId) {
        String filename = _rootDir;
        if (filename.back() == '/') {
            filename.erase(filename.size() - 1);
        }

        filename += request.path;
        if (filename.back() == '/') {
            filename += "index.html";
        }

        // Get the file and send it
        Mad<File> file(system()->fileSystem()->open(filename.c_str(), m8r::FS::FileOpenMode::Read));
        if (!file->valid()) {
            system()->print(Error::formatError(file->error().code(), 
                                    "******** HTTPServer: unable to open '%s'", filename.c_str()).c_str());
            return;
        }
        
        int32_t size = file->size();

        system()->printf("******** HTTPServer: sending '%s'\n", filename.c_str());

        sendResponseHeader(connectionId, size);
        
        char buf[100];
        while (1) {
            int32_t result = file->read(buf, 100);
            if (result < 0) {
                // FIXME: Report error
                break;
            }
            if (result > 0) {                
                _socket->send(connectionId, buf, result);
            }
            
            if (result < 100) {
                break;
            }
        }
        
        file->close();
    });
}
