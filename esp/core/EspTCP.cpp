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

#include "SystemInterface.h"

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
    err_t err = tcp_bind(_listenpcb, IP_ADDR_ANY, port);
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
            fireEventTask(TCPDelegate::Event::Connected, &it - _clients);
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
    
    if (!buf) {
        // Disconnected
        _clients[connectionId].disconnect();
        return 0;
    }
    
    assert(buf->len == buf->tot_len);
    fireEventTask(TCPDelegate::Event::ReceivedData, connectionId, reinterpret_cast<const char*>(buf->payload), buf->len);
    tcp_recved(pcb, buf->tot_len);
    pbuf_free(buf);
    return 0;
}

err_t EspTCP::sent(tcp_pcb* pcb, u16_t len)
{
    int16_t connectionId = findConnection(pcb);
    assert(_clients[connectionId].inUse());
    
    _clients[connectionId].sent(len);
    fireEventTask(TCPDelegate::Event::SentData, connectionId);
    return 0;
}

void EspTCP::Client::send(const char* data, uint16_t length)
{
    if (!length) {
        length = strlen(data);
    }

    if (_sending) {
        _buffer += String(data, length);
        return;
    }

    _sending = true;
    
    uint16_t maxSize = tcp_sndbuf(_pcb);

    if (maxSize < length) {
        // Put the remainder in the buffer
        _buffer = String(data + maxSize, length - maxSize);
        length = maxSize;
    }
    int8_t result = tcp_write(_pcb, data, length, 0);
    if (result != 0) {
        SystemInterface::shared()->printf("TCP ERROR(%d): failed to send %d bytes to port %d\n", result, length, _pcb->local_port);
    }
}

void EspTCP::Client::sent(uint16_t len)
{
    assert(_sending);

    if (!_buffer.empty()) {
        uint16_t maxSize = tcp_sndbuf(_pcb);
        if (maxSize < _buffer.size()) {
            int8_t result = tcp_write(_pcb, _buffer.c_str(), maxSize, 0);
            _buffer = _buffer.slice(maxSize);
            return;
        }
        
        int8_t result = tcp_write(_pcb, _buffer.c_str(), _buffer.size(), 0);
        _buffer.clear();
        return;
    }
    
    _sending = false;
}

void EspTCP::Client::disconnect()
{
    tcp_abort(_pcb);
    _pcb = nullptr;
}
