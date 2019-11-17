/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "EspTCP.h"

#include "SystemInterface.h"

using namespace m8r;

void EspTCP::init(TCPDelegate* delegate, uint16_t port, IPAddr ip)
{
    TCP::init(delegate, port, ip);
    _listenpcb = tcp_new();
    tcp_arg(_listenpcb, this);
    
    if (!ip) {
        err_t err = tcp_bind(_listenpcb, IP_ADDR_ANY, port);
        _listenpcb = tcp_listen(_listenpcb);
        tcp_accept(_listenpcb, _accept);
    } else {
        ip_addr_t addr;
        addr.addr = static_cast<uint32_t>(ip);
        tcp_connect(_listenpcb, &addr, port, _clientConnected);
    }
}

EspTCP::~EspTCP()
{
    for (auto& it : _clients) {
        if (it.inUse()) {
            it.disconnect();
            _delegate->TCPevent(this, TCPDelegate::Event::SentData, &it - &(_clients[0]));
        }
    }
    tcp_close(_listenpcb);
}

err_t EspTCP::clientConnected(tcp_pcb* pcb, err_t err)
{
    assert(!_clients[0].inUse());
    _clients[0] = Client(pcb, this);
    _delegate->TCPevent(this, TCPDelegate::Event::Connected, 0);
    return 0;
}

err_t EspTCP::accept(tcp_pcb* pcb, int8_t err)
{
    tcp_accepted(_listenpcb);
    
    for (auto& it : _clients) {
        if (!it.inUse()) {
            it = Client(pcb, this);
            _delegate->TCPevent(this, TCPDelegate::Event::Connected, &it - _clients);
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
        _delegate->TCPevent(this, TCPDelegate::Event::SentData, connectionId);
        return 0;
    }
    
    assert(buf->len == buf->tot_len);
    _delegate->TCPevent(this, TCPDelegate::Event::ReceivedData, connectionId, reinterpret_cast<const char*>(buf->payload), buf->len);
    tcp_recved(pcb, buf->tot_len);
    pbuf_free(buf);
    return 0;
}

err_t EspTCP::sent(tcp_pcb* pcb, u16_t len)
{
    int16_t connectionId = findConnection(pcb);
    assert(_clients[connectionId].inUse());
    
    _clients[connectionId].sent(len);
    _delegate->TCPevent(this, TCPDelegate::Event::SentData, connectionId);
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
        system()->printf("TCP ERROR(%d): failed to send %d bytes to port %d\n", result, length, _pcb->local_port);
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
    tcp_close(_pcb);
    _pcb = nullptr;
}
