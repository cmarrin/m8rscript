/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "EspUDP.h"

#include "Esp.h"
#include "IPAddr.h"
#include "SystemInterface.h"
#include <stdlib.h>
#include <lwip/igmp.h>

using namespace m8r;

void UDP::joinMulticastGroup(IPAddr addr)
{
    ip_addr_t ifaddr;
    ifaddr.addr = static_cast<uint32_t>(addr);
    ip_addr_t multicastAddr;
    IPAddr multicastIP(224,0,0,251);
    multicastAddr.addr = static_cast<uint32_t>(multicastIP);

    igmp_joingroup(&ifaddr, &multicastAddr);
}

void UDP::leaveMulticastGroup(IPAddr addr)
{
    ip_addr_t ifaddr;
    ifaddr.addr = static_cast<uint32_t>(addr);
    ip_addr_t multicastAddr;
    IPAddr multicastIP(224,0,0,251);
    multicastAddr.addr = static_cast<uint32_t>(multicastIP);

    igmp_leavegroup(&ifaddr, &multicastAddr);
}

void EspUDP::init(UDPDelegate* delegate, uint16_t port)
{
    UDP::init(delegate, port);
    _pcb = udp_new();
    if (port) {
        udp_recv(_pcb, _recv, this);
        ip_addr addr;
        addr.addr = 0;
        err_t result = udp_bind(_pcb, &addr, port);
    }
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
    String s(reinterpret_cast<const char*>(buf->payload), buf->len);
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
        m8r::system()->printf(ROMSTR("UDP ERROR: failed to send %d bytes to port %d\n"), length, port);
    }
}

void EspUDP::disconnect()
{
    udp_remove(_pcb);
    _pcb = nullptr;
}
