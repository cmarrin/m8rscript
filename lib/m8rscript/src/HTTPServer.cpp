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
#include "SystemTime.h"

using namespace m8r;

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

void HTTPServer::start(uint16_t port, const char* rootDir)
{
}

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
