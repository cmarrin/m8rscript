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

static const char _GET[] ROMSTR_ATTR = "GET";
static const char _PUT[] ROMSTR_ATTR = "PUT";
static const char _POST[] ROMSTR_ATTR = "POST";
static const char _DELETE[] ROMSTR_ATTR = "DELETE";

struct Methods
{
    const char* str;
    HTTPServer::Method method;
};

Methods RODATA_ATTR _methods[] = {
    { _GET, HTTPServer::Method::GET },
    { _PUT, HTTPServer::Method::PUT },
    { _POST, HTTPServer::Method::POST },
    { _DELETE, HTTPServer::Method::DELETE },
};

static HTTPServer::Method toMethod(const char* s)
{
    for (const auto& it : _methods) {
        if (ROMString::strcmp(ROMString(it.str), s) == 0) {
            return it.method;
        }
    }
    return HTTPServer::Method::ANY;
}

static String toString(HTTPServer::Method method)
{
    for (const auto& it : _methods) {
        if (it.method == method) {
            return ROMString(it.str);
        }
    }
    return String("*** Unknown ***");
}

struct HTTPServer::Request
{
    Request(const String& s)
    {
        // Split into lines
        Vector<String> lines = s.split("\n");
        
        // Handle method line
        Vector<String> line = lines[0].split(" ");
        if (line.size() != 3) {
            return;
        }
        
        method = toMethod(line[0].c_str());;
        if (method == Method::ANY) {
            // Invalid
            return;
        }
        
        path = line[1];
        
        // For now we don't care about anything else
        valid = true;
        return;
    }
    
    HTTPServer::Method method;
    String path;
    String params;
    Map<String, String> headers;
    Map<String, String> query;
    Map<String, String> cookies;
    bool valid = false;
};

HTTPServer::HTTPServer(uint16_t port, const char* rootDir, bool dirAccess)
{
    Mad<TCP> socket = system()->createTCP(port, [this, rootDir](TCP* tcp, TCP::Event event, int16_t connectionId, const char* data, int16_t length)
    {
        if ((connectionId < 0 || connectionId >= TCP::MaxConnections) && event != TCP::Event::Error) {
            system()->printf(ROMSTR("******** HTTPServer Internal Error: Invalid connectionId = %d\n"), connectionId);
            _socket->disconnect(connectionId);
            return;
        }

        switch(event) {
            case TCP::Event::Connected:
                system()->printf(ROMSTR("HTTPServer: new connection, connectionId=%d, ip=%s, port=%d\n"), 
                                 connectionId, tcp->clientIPAddr(connectionId).toString().c_str(), tcp->port());
                break;
            case TCP::Event::Disconnected:
                system()->printf(ROMSTR("HTTPServer: disconnecting, connectionId=%d, ip=%s, port=%d\n"), 
                                 connectionId, tcp->clientIPAddr(connectionId).toString().c_str(), tcp->port());
                break;
            case TCP::Event::ReceivedData: {
                // FIXME: Handle incoming data
                // This might be a request (GET, PUT, POST) or upoaded data for a PUT
                String header(data, length);
                printf("******** Received HTTP data:\n%s", header.c_str());
                
                Request req(header);
                if (!req.valid) {
                    system()->printf(ROMSTR("******** HTTPServer Invalid header\n"));
                    break;
                }
                
                bool handled = false;
                
                for (const auto& it : _requestHandlers) {
                    // See if we have a path match.
                    // FIXME: For now handle only exact matches.
                    if (it._method != Method::ANY && req.method != it._method) {
                        continue;
                    }
                    
                    if (it._uri == req.path) {
                        if (!it._path.empty()) {
                            String filename = rootDir;
                            if (filename.back() == '/') {
                                filename.erase(filename.size() - 1);
                            }
                
                            filename += req.path;
                            if (filename.back() == '/') {
                                filename += "index.html";
                            }

                            // Get the file and send it
                            Mad<File> file(system()->fileSystem()->open(filename.c_str(), m8r::FS::FileOpenMode::Read));
                            if (!file->valid()) {
                                system()->print(Error::formatError(file->error().code(), 
                                                                   ROMSTR("******** HTTPServer: unable to open '%s'"), filename.c_str()).c_str());
                                break;
                            }
                            
                            int32_t size = file->size();
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
                        } else if (it._requestHandler) {
                            it._requestHandler(it._uri, req.method);
                        }
                        handled = true;
                        break;
                    }
                }
                
                if (!handled) {
                    system()->printf(ROMSTR("******** HTTPServer Error: no %s method handler for uri '%s'\n"),
                                            toString(req.method).c_str(), req.path.c_str());
                }
                break;
            }
            case TCP::Event::SentData:
                break;
            case TCP::Event::Error:
                system()->printf(ROMSTR("******** HTTPServer Error: code=%d (%s)\n"), connectionId, data);
            default:
                break;
        }
    });
    
    _socket = socket;
}

void HTTPServer::sendResponseHeader(int16_t connectionId, uint32_t size)
{
    // This is for a valid response
    String s = ROMString::format(ROMString("HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n"),
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
    return ROMString::format(ROMString("%s, %d %s %d %02d:%02d:%02d *"), 
                             el.dayString().c_str(), el.day, el.monthString().c_str(), el.year, el.hour, el.minute, el.second);
}

void HTTPServer::handleEvents()
{
    if (_socket.valid()) {
        _socket->handleEvents();
    }
}

HTTPServer* HTTPServer::on(const String& uri, RequestFunction f)
{
    _requestHandlers.emplace_back(uri, Method::ANY, f);
    return this;
}

HTTPServer* HTTPServer::on(const String& uri, Method method, RequestFunction f)
{
    _requestHandlers.emplace_back(uri, method, f);
    return this;
}

HTTPServer* HTTPServer::on(const String& uri, const String& path, bool dirAccess)
{
    _requestHandlers.emplace_back(uri, Method::ANY, path, dirAccess);
    return this;
}
