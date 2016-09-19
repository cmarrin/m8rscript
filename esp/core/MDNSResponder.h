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

#include "Esp.h"
#include "Containers.h"
#include "UDP.h"

#include <ip_addr.h>
#include <espconn.h>

namespace m8r {

class MDNSResponder {
public:
    enum class ServiceProtocol { TCP, UDP };
    
    MDNSResponder(const char* name, uint32_t broadcastInterval = 60, uint32_t ttl = 120);
    ~MDNSResponder();

    void addService(uint16_t port, const char* instance, const char* serviceType, 
                    ServiceProtocol protocol = ServiceProtocol::TCP, const char* text = nullptr);
    
private:
    enum class QuestionType {
       A = 0x0001,
       NS = 0x0002,
       CNAME = 0x0005,
       SOA = 0x0006,
       WKS = 0x000b,
       PTR = 0x000c,
       MX = 0x000f,
       TXT = 0x0010,
       SRV = 0x0021,
       AAAA = 0x001c,
    };

    struct MDNSHeader {
       uint16_t    xid;
       uint8_t     recursionDesired:1;
       uint8_t     truncated:1;
       uint8_t     authoritiveAnswer:1;
       uint8_t     opCode:4;
       uint8_t     queryResponse:1;
       uint8_t     responseCode:4;
       uint8_t     checkingDisabled:1;
       uint8_t     authenticatedData:1;
       uint8_t     zReserved:1;
       uint8_t     recursionAvailable:1;
       uint16_t    queryCount;
       uint16_t    answerCount;
       uint16_t    authorityCount;
       uint16_t    additionalCount;
    } __attribute__((__packed__));

    struct ServiceRecord {
        ServiceRecord(uint16_t port, const char* instance, const char* serviceType, ServiceProtocol protocol, const char* text)
            : _port(port)
            , _instance(String(instance))
            , _serviceType(String(serviceType))
            , _protocol(protocol)
            , _text(String(text))
        { }
        
        uint16_t _port;
        ServiceProtocol _protocol;
        String _instance;
        String _serviceType;
        String _text;
    };
    
    void receivedData(const char* data, uint16_t length);

    void writeHeader();
    void writeA();
    void writePTR();
    void writeSRV();
    void writeTXT();
    
    void sendReply();
    
    static void broadcastCB(void* arg) { reinterpret_cast<MDNSResponder*>(arg)->broadcast(); } 
    
    void broadcast();

    String _hostname;
    std::vector<ServiceRecord> _services;
    uint32_t _ttl = 0;
    std::vector<uint8_t> _replyBuffer;
    
    class MyUDP : public esp::UDP {
    public:
        MyUDP(uint16_t port, MDNSResponder* responder)
            : UDP(port)
            , _responder(responder)
        { }
        
        virtual void receivedData(const char* data, uint16_t length) override
        {
            _responder->receivedData(data, length);
        }
        
        virtual void sentData() override { }
    
    private:
        MDNSResponder* _responder;
    };
    
    MyUDP* _udp;
};

}
