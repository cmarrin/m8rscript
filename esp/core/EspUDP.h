/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "UDP.h"

extern "C" {
#include <lwip/init.h>
#include <lwip/udp.h>





//#include <ip_addr.h>
//#include <espconn.h>
//#include <osapi.h>
//#include <ets_sys.h>
//#include "user_interface.h"
}

namespace m8r {

class EspUDP : public UDP {
public:
    EspUDP(UDPDelegate*, uint16_t = 0);
    virtual ~EspUDP();
    
    static void joinMulticastGroup(IPAddr);
    static void leaveMulticastGroup(IPAddr);
    
    virtual void send(IPAddr, uint16_t port, const char* data, uint16_t length = 0) override;
    virtual void disconnect() override;
    
private:    
    static void _recv(void *arg, struct udp_pcb *pcb, struct pbuf *buf,  ip_addr_t *addr, u16_t port) { reinterpret_cast<EspUDP*>(arg)->recv(pcb, buf, addr, port); }

    void recv(udp_pcb*, pbuf*, ip_addr_t *addr, u16_t port);

    udp_pcb* _pcb;
};

}
