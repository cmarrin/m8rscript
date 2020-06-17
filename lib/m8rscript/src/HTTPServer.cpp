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
#include "MString.h"
#include "SystemInterface.h"
#include "SystemTime.h"

using namespace m8r;

HTTPServer::HTTPServer(uint16_t port, const char* rootDir)
{
    Mad<TCP> socket = system()->createTCP(port, [this](TCP* tcp, TCP::Event event, int16_t connectionId, const char* data, int16_t length)
    {
        if (connectionId < 0 || connectionId >= TCP::MaxConnections) {
            system()->printf(ROMSTR("******** HTTPServer Internal Error: Invalid connectionId = %d\n"), connectionId);
            _socket->disconnect(connectionId);
            return;
        }

        switch(event) {
            case TCP::Event::Connected:
                system()->printf(ROMSTR("HTTPServer: new connection, connectionId=%d, ip=%s, port=%d\n"), 
                                 connectionId, tcp->clientIPAddr(connectionId).toString().c_str(), tcp->port());
                _socket->send(connectionId, "HTTP/1.0 200 OK\r\nContent-Length: 16\r\n\r\nHello HTTP World");

                break;
            case TCP::Event::Disconnected:
                system()->printf(ROMSTR("HTTPServer: disconnecting, connectionId=%d, ip=%s, port=%d\n"), 
                                 connectionId, tcp->clientIPAddr(connectionId).toString().c_str(), tcp->port());
                break;
            case TCP::Event::ReceivedData:
                // FIXME: Handle incoming data
                // This might be a request (GET, PUT, POST) or upoaded data for a PUT
                printf("******** Received HTTP data:\n%s", String(data, length).c_str());
                break;
            case TCP::Event::SentData:
                break;
            case TCP::Event::Error:
                system()->printf(ROMSTR("******** HTTPServer Error for connectionId = %d (%s)\n"), connectionId, data);
            default:
                break;
        }
    });
    
    _socket = socket;
}

struct HTTPServer::Request
{
    String method;
    String path;
    String params;
    Map<String, String> headers;
    Map<String, String> query;
    Map<String, String> cookies;
};

struct HTTPServer::Response
{
    void send(String str)
    {
       body += str;
    }
    
    void send(const char* str)
    {
       body += str;
    }

    int code = 200;
    String phrase = "OK";
    String type = "text/html";
    String date = dateString();
    String body;

};

String HTTPServer::dateString()
{
    // Format is <abbrev wkday>, <day> <abbrev month> <year> <hr>:<min>:<sec> <tz name>
    Time now = Time::now();
    Time::Elements el;
    now.elements(el);
    
    // FIXME: Get timezone
    return ROMString::format(ROMString("%s, %d %s %d %02d:%02d:02d *"), 
                             el.dayString().c_str(), el.day, el.monthString().c_str(), el.year, el.hour, el.minute, el.second);
}
