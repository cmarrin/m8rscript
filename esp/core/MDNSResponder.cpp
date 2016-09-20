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

#include "MDNSResponder.h"

#include <cstring>
#include <cstdlib>

extern "C" {
#include "user_interface.h"
}

using namespace m8r;

#define MDNSRESP_DEBUG

static os_timer_t bc_timer;

MDNSResponder::MDNSResponder(const char* name, uint32_t broadcastInterval, uint32_t ttl)
{
	_ttl = ttl;
    _hostname = name;
    
    esp::UDP::joinMulticastGroup({ 224,0,0,251 });
    _udp = new MyUDP(5353, this);

	os_timer_disarm(&bc_timer);
	if (broadcastInterval > 0) {
		os_timer_setfn(&bc_timer, (os_timer_func_t *)broadcastCB, this);
		os_timer_arm(&bc_timer, broadcastInterval * 1000, true);
	}
}

MDNSResponder::~MDNSResponder()
{
	os_timer_disarm(&bc_timer);
    delete _udp;
    esp::UDP::leaveMulticastGroup({ 224,0,0,251 });
}

const char* decodeNameStrings(const char* data, uint32_t dataLen, std::vector<String>& names, const char* p)
{
    names.clear();
    
    const char* pointerReturn = nullptr;
    while(1) {
        uint32_t size = static_cast<uint32_t>(*p++);
        if ((size & 0xc0) == 0) {
            // this is a direct label
            names.push_back(String(reinterpret_cast<const char*>(p), size));
            p += size;
            if (*p == 0) {
                ++p;
                break;
            }
        } else {
            // This is a pointer label
            if (pointerReturn) {
                break;
            }
            uint32_t offset = ((size & ~0xc0) << 8) | *p++;
            pointerReturn = p;
            p = data + offset;
        }
    }
    return pointerReturn ?: p;
}

static inline uint16_t uint16FromBuf(const char* buf)
{
    return (static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]);
}

void MDNSResponder::receivedData(const char* data, uint16_t length)
{
    if (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0) {
        // only queries
        return;
    }

    int qcount = data[5]; // purposely ignore qcount > 255
    const char* p = data+12;
    std::vector<String> names;
    
    while(qcount-- > 0) {
        p = decodeNameStrings(data, length, names, p);
        QuestionType qtype = static_cast<QuestionType>(uint16FromBuf(p));
        uint16_t qclass = uint16FromBuf(p + 2);
        p += 4;
        
        if ((qclass & 0x7fff) != 1 || names.size() < 2) {
            continue;
        }
        
        String serviceName;
        bool haveService = false;
        ServiceProtocol protocol;
        
        if (names[0][0] == '_') {
            // this is a service name
            serviceName = names[0].slice(1);
            haveService = true;
            if (names[1] == "_tcp") {
                protocol = ServiceProtocol::TCP;
            } else if (names[1] == "_udp") {
                protocol = ServiceProtocol::UDP;
            } else {
                // not a recognized protocol
                continue;
            }
        } else if (names[0] != _hostname || names[1] != "local") {
            // This is a hostname and it's not ours
            continue;
        }
        
        int32_t serviceIndex = -1;
        
        if (haveService) {
            for (int32_t i = 0; i < _services.size(); ++i) {
                if (_services[i]._serviceType == serviceName && _services[i]._protocol == protocol) {
                    serviceIndex = i;
                    break;
                }
            }
        }

#ifdef MDNSRESP_DEBUG
        if (serviceIndex >= 0) {
            os_printf ("Got one of our services: %s.%s", names[0].c_str(), names[1].c_str());
        } else {
            os_printf ("Got our hostname: %s.local", names[0].c_str());
        }
        os_printf (" - qtype=%d qclass=%d\n", qtype, qclass);
#endif
        uint8_t count = 0;
        switch(qtype) {
            case QuestionType::A: count = 1; break;
            case QuestionType::TXT: count = 1; break;
            case QuestionType::SRV: count = 2; break;
            case QuestionType::PTR: count = 4; break;
        }
        if (!count) {
            return;
        }
        
        writeHeader(count);
        
        if (qtype == QuestionType::PTR) {
            writePTR();
        }
        if (qtype == QuestionType::TXT || qtype == QuestionType::PTR) {
            writeTXT();
        }
        if (qtype == QuestionType::SRV || qtype == QuestionType::PTR) {
            writeSRV();
        }
		if (qtype == QuestionType::A || qtype == QuestionType::SRV || qtype == QuestionType::PTR) {
            writeA();
        }
        
        sendReply();
	}
}

void MDNSResponder::broadcast()
{
    writeHeader(4);
    writePTR();
    writeTXT();
    writeSRV();
    writeA();
    sendReply();
}

void MDNSResponder::addService(uint16_t port, const char* instance, const char* serviceType, ServiceProtocol protocol, const char* text)
{
    _services.emplace_back(port, instance, serviceType, protocol, text);
}

void MDNSResponder::writeHeader(uint8_t count)
{
    _replyBuffer.clear();
    
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0x84); // Response + Authoritative Answer
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(count); // Answer count
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
}

void MDNSResponder::write(const char* s, size_t length)
{
    _replyBuffer.push_back(length);
    for (size_t i = 0; i < length; ++i) {
        _replyBuffer.push_back(s[i]);
    }
}

void MDNSResponder::writeHostname(bool terminate)
{
    write(_hostname.c_str(), _hostname.length());
    write("local", 5);
    if (terminate) {
        _replyBuffer.push_back(0);
    }
}

void MDNSResponder::writeA()
{
    writeHostname(true);
    write(static_cast<uint16_t>(QuestionType::A));
    write(QClassIN);
    write(_ttl);
    write(static_cast<uint16_t>(4)); // length of IP
    write(static_cast<uint32_t>(IPAddr::myIPAddr()));
}

void MDNSResponder::writePTR()
{
}

void MDNSResponder::writeSRV()
{
}

void MDNSResponder::writeTXT()
{
}

void MDNSResponder::sendReply()
{
    if (_replyBuffer.empty()) {
        return;
    }
    
    _udp->send({ 224,0,0,251 }, 5353, reinterpret_cast<char*>(&(_replyBuffer[0])), _replyBuffer.size());
    _replyBuffer.clear();
}
