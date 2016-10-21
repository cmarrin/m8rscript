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

#include "UDP.h"

#include <stdlib.h>

using namespace m8r;

IPAddr IPAddr::myIPAddr()
{
    struct ip_info info;
    wifi_get_ip_info(STATION_IF, &info);
    return IPAddr(info.ip.addr);
}

void UDP::joinMulticastGroup(IPAddr addr)
{
    struct ip_addr mDNSmulticast;
    struct ip_addr any;
    mDNSmulticast.addr = addr;
    any.addr = IPADDR_ANY;
	espconn_igmp_join(&any, &mDNSmulticast);
}

void UDP::leaveMulticastGroup(IPAddr addr)
{
    struct ip_addr mDNSmulticast;
    struct ip_addr any;
    mDNSmulticast.addr = addr;
    any.addr = IPADDR_ANY;
	espconn_igmp_leave(&any, &mDNSmulticast);
}

UDP::UDP(uint16_t port)
{
	_conn.type = ESPCONN_UDP;
	_conn.state = ESPCONN_NONE;
	_conn.proto.udp = &_udp;
	_udp.local_port = port;
	_conn.reverse = this;

	espconn_regist_recvcb(&_conn, receiveCB);
	espconn_create(&_conn);
}

UDP::~UDP()
{
	espconn_disconnect(&_conn);
	espconn_delete(&_conn);
}

void UDP::send(IPAddr addr, uint16_t port, const char* data, uint16_t length)
{
    if (!length) {
        length = strlen(data);
    }

	_conn.proto.udp->remote_ip[0] = addr[0];
	_conn.proto.udp->remote_ip[1] = addr[1];
	_conn.proto.udp->remote_ip[2] = addr[2];
	_conn.proto.udp->remote_ip[3] = addr[3];
	_conn.proto.udp->remote_port = port;
	_conn.proto.udp->local_port = port;
    
    int8_t result = espconn_send(&_conn, reinterpret_cast<uint8_t*>(const_cast<char*>(data)), length);
    if (result != 0) {
        os_printf("UDP ERROR: failed to send %d bytes to port %d\n", length, _conn.proto.udp->local_port);
    }
}

void UDP::receiveCB(void* arg, char* data, uint16_t length)
{
    struct espconn* conn = (struct espconn *) arg;

    reinterpret_cast<UDP*>(conn->reverse)->receivedData(data, length);
}

void UDP::sentCB(void* arg)
{
    struct espconn* conn = (struct espconn *) arg;
    reinterpret_cast<UDP*>(conn->reverse)->sentData();
}
