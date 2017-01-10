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

#include "EspTCP.h"

using namespace m8r;

TCP* TCP::create(TCPDelegate* delegate, uint16_t port)
{
    return new EspTCP(delegate, port, IPAddr());
}

TCP* TCP::create(TCPDelegate* delegate, uint16_t port, IPAddr ip)
{
    return new EspTCP(delegate, port, ip);
}

EspTCP::EspTCP(TCPDelegate* delegate, uint16_t port, IPAddr ip)
    : TCP(delegate, port, ip)
{
    assert(!ip); // client not yet supported

    _listenpcb = tcp_new();
    tcp_bind(_listenpcb, IP_ADDR_ANY, port);
    _listenpcb = tcp_listen(_listenpcb);
    tcp_arg(_listenpcb, this);
    tcp_accept(_listenpcb, _accept);
}

EspTCP::~EspTCP()
{
}

err_t EspTCP::accept(tcp_pcb* pcb, int8_t err)
{
    tcp_accepted(_listenpcb);
    
    for (auto& it : _clients) {
        if (!it.inUse()) {
            it = Client(pcb, this);
            _delegate->TCPconnected(this, &it - _clients);
            return 0;
        }
    }
    tcp_abort(pcb);
    return 0;
}

int16_t EspTCP::findConnection(tcp_pcb* pcb)
{
    for (auto& it : _clients) {
        if (it.matches(pcb)) {
            return &it - _clients;
        }
    }
    return -1;
}

err_t EspTCP::recv(tcp_pcb* pcb, pbuf* buf, int8_t err)
{
    int16_t connectionId = findConnection(pcb);
    if (connectionId < 0) {
        return -1;
    }
    
    assert(buf->len == buf->tot_len);
    _delegate->TCPreceivedData(this, connectionId, reinterpret_cast<const char*>(buf->payload), buf->len);
    tcp_recved(pcb, buf->tot_len);
    pbuf_free(buf);
    return 0;
}

err_t EspTCP::sent(tcp_pcb* pcb, u16_t len)
{
    // FIXME: Implement
    
    return 0;
}

void EspTCP::Client::send(const char* data, uint16_t length)
{
    // FIXME: Implement
    
//    if (!_connected) {
//        return;
//    }
//    if (!length) {
//        length = strlen(data);
//    }
//
//    if (_sending) {
//        _buffer += String(data, length);
//        return;
//    }
//
//    _sending = true;
//    //uint16_t maxSize = tcp_sndbuf(
//    int8_t result = 0;
//    if (result != 0) {
//        os_printf("TCP ERROR: failed to send %d bytes to port %d\n", length, _port);
//    }
}

void EspTCP::Client::disconnect()
{
    // FIXME: Implement
    
//    if (!_connected) {
//        return;
//    }
}
