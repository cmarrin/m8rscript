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

#include "EspUDP.h"

#include "Esp.h"
#include "IPAddr.h"
#include <stdlib.h>

using namespace m8r;

UDP* UDP::create(UDPDelegate* delegate, uint16_t port)
{
    return new EspUDP(delegate, port);
}

void UDP::joinMulticastGroup(IPAddr addr)
{
//    struct ip_addr mDNSmulticast;
//    struct ip_addr any;
//    mDNSmulticast.addr = static_cast<uint32_t>(addr);
//    any.addr = IPADDR_ANY;
//	espconn_igmp_join(&any, &mDNSmulticast);
}

void UDP::leaveMulticastGroup(IPAddr addr)
{
//    struct ip_addr mDNSmulticast;
//    struct ip_addr any;
//    mDNSmulticast.addr = addr;
//    any.addr = IPADDR_ANY;
//	espconn_igmp_leave(&any, &mDNSmulticast);
}

EspUDP::EspUDP(UDPDelegate* delegate, uint16_t port)
    : UDP(delegate, port)
{
    _pcb = udp_new();
    if (port) {
        err_t result = udp_bind(_pcb, IP_ADDR_ANY, port);
    }
    udp_recv(_pcb, _recv, this);
}

EspUDP::~EspUDP()
{
    udp_remove(_pcb);
}

void EspUDP::recv(udp_pcb* pcb, pbuf* buf, ip_addr_t *addr, u16_t port)
{
    if (!buf) {
        // Disconnected
        return;
    }
    
    assert(buf->len == buf->tot_len);
    _delegate->UDPevent(this, UDPDelegate::Event::ReceivedData, reinterpret_cast<const char*>(buf->payload), buf->len);
    pbuf_free(buf);
}

void EspUDP::send(IPAddr addr, uint16_t port, const char* data, uint16_t length)
{
    if (!length) {
        length = strlen(data);
    }
    
    pbuf* buf = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
    memcpy(buf->payload, data, length);
    ip_addr_t ip;
    IP4_ADDR(&ip, addr[0], addr[1], addr[2], addr[3]);
    err_t result = udp_sendto(_pcb, buf, &ip, port);
    pbuf_free(buf);
    if (result != 0) {
        os_printf("UDP ERROR: failed to send %d bytes to port %d\n", length, port);
    }
}

void EspUDP::disconnect()
{
    udp_remove(_pcb);
    _pcb = nullptr;
}
