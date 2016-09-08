/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
	  
    - Redistributions in binary form must reproduce the above copyright 
	  notice, this list of conditions and the following disclaimer in the 
	  documentation and/or other materials provided with the distribution.
	  
    - Neither the name of the <ORGANIZATION> nor the names of its 
	  contributors may be used to endorse or promote products derived from 
	  this software without specific prior written permission.
	  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "TCP.h"

#include "Esp.h"
#include <stdlib.h>

using namespace esp;

TCP::TCP(uint16_t port)
{
    _conn.type = ESPCONN_TCP;
    _conn.state = ESPCONN_NONE;
    _conn.proto.tcp = &_tcp;
    _conn.proto.tcp->local_port = port;
    _conn.reverse = this;
    
    espconn_regist_connectcb(&_conn, connectCB);
    int8_t result = espconn_accept(&_conn);
    if (result != 0) {
        os_printf("ERROR: espconn_accept (%d)\n", result);
        return;
    }
    
    os_printf("TCP: accepting connections on port %d\n", port);
}

TCP::~TCP()
{
    espconn_abort(&_conn);
    espconn_disconnect(&_conn);
}

void TCP::send(const char* data, uint16_t length)
{
    if (!length) {
        length = strlen(data);
    }

    int8_t result = espconn_send(&_conn, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), length);
    if (result != 0) {
        os_printf("TCP ERROR: failed to send %d bytes to port %d\n", length, _conn.proto.tcp->local_port);
    }
}

void TCP::disconnect()
{
    espconn_disconnect(&_conn);
}

void TCP::connectCB(void* arg)
{
    struct espconn* conn = (struct espconn *) arg;

    os_printf("TCP: connection established to port %d\n", conn->proto.tcp->local_port);

    //espconn_regist_time(conn, 60, 1);
    espconn_regist_recvcb(conn, receiveCB);
    espconn_regist_reconcb(conn, reconnectCB);
    espconn_regist_disconcb(conn, disconnectCB);
    espconn_regist_sentcb(conn, sentCB);
    
    reinterpret_cast<TCP*>(conn->reverse)->connected();
}

void TCP::disconnectCB(void* arg)
{
    struct espconn* conn = (struct espconn *) arg;

    os_printf("TCP: disconnected from port %d\n", conn->proto.tcp->local_port);

    reinterpret_cast<TCP*>(conn->reverse)->disconnected();
}

void TCP::reconnectCB(void* arg, int8_t error)
{
    struct espconn* conn = (struct espconn *) arg;

    os_printf("TCP: reconnected to port %d, error=%d\n", conn->proto.tcp->local_port, error);
}

void TCP::receiveCB(void* arg, char* data, uint16_t length)
{
    struct espconn* conn = (struct espconn *) arg;

    os_printf("TCP: received %d bytes from port %d\n", length, conn->proto.tcp->local_port);

    reinterpret_cast<TCP*>(conn->reverse)->receivedData(data, length);
}

void TCP::sentCB(void* arg)
{
    struct espconn* conn = (struct espconn *) arg;
    reinterpret_cast<TCP*>(conn->reverse)->sentData();
}
