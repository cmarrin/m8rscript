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

#pragma once

#include "TaskManager.h"
#include "UDP.h"
#include <cstdint>
#include <cstring>

namespace m8r {

class TCP;

class TCPDelegate {
public:
    enum class Event { Connected, Reconnected, Disconnected, ReceivedData, SentData };
    
    virtual void TCPevent(TCP*, Event, int16_t connectionId, const char* data = nullptr, uint16_t length = 0) = 0;
};

class TCP {
    friend class MyEventTask;
    
public:
    static constexpr int MaxConnections = 4;
    static constexpr uint32_t DefaultTimeout = 7200;
     
    static TCP* create(TCPDelegate*, uint16_t port);
    static TCP* create(TCPDelegate*, uint16_t port, IPAddr ip);
    virtual ~TCP()
    {
        while (_eventsInFlight) {
            MyEventTask* task = _eventsInFlight;
            _eventsInFlight = _eventsInFlight->_next;
            task->remove();
            delete task;
        }
        while (_eventPool) {
            MyEventTask* task = _eventPool;
            _eventPool = _eventPool->_next;
            delete task;
        }
    }
    
    virtual void send(int16_t connectionId, char c) = 0;
    virtual void send(int16_t connectionId, const char* data, uint16_t length = 0) = 0;
    virtual void disconnect(int16_t connectionId) = 0;

protected:
    struct MyEventTask : public Task {
        MyEventTask(TCP* tcp, TCPDelegate* delegate) : _tcp(tcp), _delegate(delegate) { }
        
        void fire(TCPDelegate::Event event, int16_t connectionId, const char* data, uint16_t length)
        {
            _event = event;
            _connectionId = connectionId;
            _data = new char[length];
            memcpy(_data, data, length);
            _length = length;
            runOnce();
        }
        
        void release()
        {
            delete [ ] _data;
        }
        
        virtual bool execute() override
        {
            _delegate->TCPevent(_tcp, _event, _connectionId, _data, _length);
            delete [ ] _data;
            _tcp->releaseEventTask(this);
            return true;
        }
        
        TCP* _tcp;
        TCPDelegate* _delegate;
        
        TCPDelegate::Event _event;
        int16_t _connectionId;
        char* _data;
        uint16_t _length;
        
        MyEventTask* _next = nullptr;
    };
    
    void fireEventTask(TCPDelegate::Event event, int16_t connectionId, const char* data = nullptr, uint16_t length = 0)
    {
        if (!_eventPool) {
            _eventPool = new MyEventTask(this, _delegate);
        }
        MyEventTask* task = _eventPool;
        _eventPool = task->_next;
        
        task->_next = _eventsInFlight;
        _eventsInFlight = task;
        task->fire(event, connectionId, data, length);
    }
    
    void releaseEventTask(MyEventTask* task)
    {
        MyEventTask* prev = nullptr;
        for (MyEventTask* t = _eventsInFlight; t; t = t->_next) {
            if (t == task) {
                if (!prev) {
                    _eventsInFlight = task->_next;
                } else {
                    prev->_next = task->_next;
                }
                break;
            }
        }
        task->_next = _eventPool;
        _eventPool = task;
    }

    MyEventTask* _eventPool = nullptr;
    MyEventTask* _eventsInFlight = nullptr;
    
    TCP(TCPDelegate* delegate, uint16_t port, IPAddr ip = IPAddr()) : _delegate(delegate), _ip(ip), _port(port) { }

    TCPDelegate* _delegate;
    IPAddr _ip;
    uint16_t _port;
};

}
